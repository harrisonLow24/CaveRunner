#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define MAP_WIDTH 24
#define MAP_HEIGHT 24
#define FOV 60
#define MAX_DIALOG_LENGTH 1024
#define MAX_NPCS 10
#define MAX_DIALOG_OPTIONS 4
#define MAX_DIALOG_NODES 20

typedef struct DialogOption {
    char text[MAX_DIALOG_LENGTH];
    int next_node;
    void (*action)(void*);
} DialogOption;

typedef struct DialogNode {
    char text[MAX_DIALOG_LENGTH];
    DialogOption options[MAX_DIALOG_OPTIONS];
    int option_count;
    int is_end_node;
} DialogNode;

typedef struct DialogTree {
    DialogNode nodes[MAX_DIALOG_NODES];
    int current_node;
    int node_count;
} DialogTree;

typedef struct {
    float x;
    float y;
    float angle;
    float height;
    int health;
    char current_dialog[MAX_DIALOG_LENGTH];
    int in_dialog;
    int selected_option;
} Player;

typedef struct {
    float x;
    float y;
    char name[32];
    char dialog[MAX_DIALOG_LENGTH];
    int active;
    DialogTree* dialog_tree;
    int has_been_talked_to;
    int quest_giver;
    int quest_id;
} NPC;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    int is_running;
    Player player;
    NPC npcs[MAX_NPCS];
    const Uint8 *keyboard_state;
} GameState;

// world map
int world_map[MAP_WIDTH][MAP_HEIGHT] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,2,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// create a dialog node
DialogNode create_dialog_node(const char* text, int is_end) {
    DialogNode node;
    strncpy(node.text, text, MAX_DIALOG_LENGTH - 1);
    node.option_count = 0;
    node.is_end_node = is_end;
    return node;
}

// add option to a dialog node
void add_dialog_option(DialogNode* node, const char* text, int next_node, void (*action)(void*)) {
    if (node->option_count < MAX_DIALOG_OPTIONS) {
        DialogOption* option = &node->options[node->option_count];
        strncpy(option->text, text, MAX_DIALOG_LENGTH - 1);
        option->next_node = next_node;
        option->action = action;
        node->option_count++;
    }
}

// create merchant's dialog tree
DialogTree* create_merchant_dialog() {
    DialogTree* tree = (DialogTree*)malloc(sizeof(DialogTree));
    tree->current_node = 0;
    tree->node_count = 0;

    // initial greeting
    DialogNode greeting = create_dialog_node("Greetings, traveler! I am Marcus, a merchant from the eastern lands. These are dark times to be wandering alone...", 0);
    add_dialog_option(&greeting, "Dark times? What do you mean?", 1, NULL);
    add_dialog_option(&greeting, "What goods do you have for sale?", 2, NULL);
    add_dialog_option(&greeting, "Goodbye.", 3, NULL);
    tree->nodes[tree->node_count++] = greeting;

    // mode 1: dark times explanation
    DialogNode dark_times = create_dialog_node("Haven't you heard? The ancient seals are breaking. The old ones stir in their slumber. Some say it's just tales, but I've seen things... strange things in my travels.", 0);
    add_dialog_option(&dark_times, "Tell me more about these seals.", 4, NULL);
    add_dialog_option(&dark_times, "Sounds like superstition to me.", 5, NULL);
    add_dialog_option(&dark_times, "Let's talk about something else.", 0, NULL);
    tree->nodes[tree->node_count++] = dark_times;

    // mode 2: shop dialog
    DialogNode shop = create_dialog_node("Ah, a potential customer! Though I must admit, my stock is limited these days. The roads are too dangerous for regular shipments.", 0);
    add_dialog_option(&shop, "Why are the roads dangerous?", 6, NULL);
    add_dialog_option(&shop, "I'll come back later.", 0, NULL);
    tree->nodes[tree->node_count++] = shop;

    // mode 3: goodbye
    DialogNode goodbye = create_dialog_node("Stay safe on your travels. And remember, if you see any strange symbols glowing in the dark... run.", 1);
    tree->nodes[tree->node_count++] = goodbye;

    // mode 4: seals explanation
    DialogNode seals = create_dialog_node("The seals were created by the ancient mages to contain... something. The records are unclear. But every thousand years, they must be renewed. And wouldn't you know it - it's been nearly a thousand years.", 0);
    add_dialog_option(&seals, "How can I help?", 7, NULL);
    add_dialog_option(&seals, "That's someone else's problem.", 3, NULL);
    tree->nodes[tree->node_count++] = seals;

    // mode 5: skepticism
    DialogNode skeptic = create_dialog_node("*chuckles* Skepticism is healthy, friend. I was like you once. But after what I saw in the ruins of Khaldar... Well, let's just say there are things that can't be unexplained.", 0);
    add_dialog_option(&skeptic, "What did you see?", 8, NULL);
    add_dialog_option(&skeptic, "I should go.", 3, NULL);
    tree->nodes[tree->node_count++] = skeptic;

    // node 6: dangerous roads
    DialogNode roads = create_dialog_node("Bandits? Hah, if only. No, something's driving the creatures from the deep forests onto the trading routes. Even the bandits have abandoned their hideouts.", 0);
    add_dialog_option(&roads, "Tell me about the creatures.", 9, NULL);
    add_dialog_option(&roads, "Back to trading.", 2, NULL);
    tree->nodes[tree->node_count++] = roads;

    // node 7: quest offer
    DialogNode quest = create_dialog_node("*lowers voice* If you're serious about helping, I know of an ancient watchtower nearby. The seal there needs checking. But be warned - you won't be the first to try.", 0);
    add_dialog_option(&quest, "I accept this quest.", 10, NULL);
    add_dialog_option(&quest, "I need to prepare first.", 0, NULL);
    tree->nodes[tree->node_count++] = quest;

    // node 8: khaldar story
    DialogNode khaldar = create_dialog_node("Shadows that moved against the wind. Writing on the walls that changed when you weren't looking. And the whispers... *shudders* Let's speak of more pleasant things.", 0);
    add_dialog_option(&khaldar, "Return to main topics.", 0, NULL);
    tree->nodes[tree->node_count++] = khaldar;

    // node 9: creatures
    DialogNode creatures = create_dialog_node("They look like normal forest beasts at first glance - wolves, bears, elk. But there's something wrong about them. Their movements, their eyes... And they're all fleeing from something deeper in the forest.", 0);
    add_dialog_option(&creatures, "Back to main topics.", 0, NULL);
    tree->nodes[tree->node_count++] = creatures;

    // node 10: quest accepted
    DialogNode quest_accepted = create_dialog_node("The watchtower lies to the north, through the ancient grove. Look for the broken stone archway. And remember - if you see glowing symbols, you're close. May the old gods watch over you.", 1);
    tree->nodes[tree->node_count++] = quest_accepted;

    return tree;
}

void init_player(Player *player) {
    player->x = 2.0f;
    player->y = 2.0f;
    player->angle = 0.0f;
    player->height = 0.5f;
    player->health = 100;
    memset(player->current_dialog, 0, MAX_DIALOG_LENGTH);
    player->in_dialog = 0;
    player->selected_option = 0;
}

void init_npcs(NPC npcs[]) {
    int npc_count = 0;
    for(int y = 0; y < MAP_HEIGHT; y++) {
        for(int x = 0; x < MAP_WIDTH; x++) {
            if(world_map[y][x] == 2 && npc_count < MAX_NPCS) {
                npcs[npc_count].x = x + 0.5f;
                npcs[npc_count].y = y + 0.5f;
                
                // initialize first NPC as merchant
                if (npc_count == 0) {
                    strcpy(npcs[npc_count].name, "Marcus the Merchant");
                    npcs[npc_count].dialog_tree = create_merchant_dialog();
                    npcs[npc_count].has_been_talked_to = 0;
                    npcs[npc_count].quest_giver = 1;
                    npcs[npc_count].quest_id = 1;
                } else {
                    sprintf(npcs[npc_count].name, "Villager %d", npc_count);
                    npcs[npc_count].dialog_tree = NULL;
                }
                
                npcs[npc_count].active = 1;
                npc_count++;
            }
        }
    }
}

void render_dialog_box(GameState *game) {
    if (!game->player.in_dialog) return;

    // find NPC player is talking to
    NPC *talking_npc = NULL;
    for (int i = 0; i < MAX_NPCS; i++) {
        if (game->npcs[i].active) {
            float dx = game->npcs[i].x - game->player.x;
            float dy = game->npcs[i].y - game->player.y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 2.0f) {
                talking_npc = &game->npcs[i];
                break;
            }
        }
    }

    if (!talking_npc || !talking_npc->dialog_tree) return;

    // draw main dialog box
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 200);
    SDL_Rect dialog_box = {
        SCREEN_WIDTH / 6,
        SCREEN_HEIGHT - 300,
        SCREEN_WIDTH * 2 / 3,
        250
    };
    SDL_RenderFillRect(game->renderer, &dialog_box);

    // draw NPC name box
    SDL_SetRenderDrawColor(game->renderer, 50, 50, 50, 255);
    SDL_Rect name_box = {
        SCREEN_WIDTH / 6,
        SCREEN_HEIGHT - 300 - 30,
        200,
        30
    };
    SDL_RenderFillRect(game->renderer, &name_box);

    // get current dialog node
    DialogNode current_node = talking_npc->dialog_tree->nodes[talking_npc->dialog_tree->current_node];

    // draw NPC name
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
    int char_width = 12;
    int char_height = 20;
    for (size_t i = 0; i < strlen(talking_npc->name); i++) {
        SDL_Rect char_rect = {
            SCREEN_WIDTH / 6 + 10 + (i * char_width),
            SCREEN_HEIGHT - 300 - 25,
            char_width,
            char_height
        };
        SDL_RenderFillRect(game->renderer, &char_rect);
    }

    // draw main dialog text
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
    int line_length = 0;
    int line_number = 0;
    for (size_t i = 0; i < strlen(current_node.text); i++) {
        if (line_length >= 60 || current_node.text[i] == '\n') { // Word wrap
            line_length = 0;
            line_number++;
            if (current_node.text[i] == '\n') continue;
        }
        SDL_Rect char_rect = {
            SCREEN_WIDTH / 6 + 20 + (line_length * char_width),
            SCREEN_HEIGHT - 280 + (line_number * (char_height + 5)),
            char_width,
            char_height
        };
        SDL_RenderFillRect(game->renderer, &char_rect);
        line_length++;
    }

    // draw dialog options
    for (int i = 0; i < current_node.option_count; i++) {
        SDL_SetRenderDrawColor(game->renderer, 
            i == game->player.selected_option ? 100 : 50,
            i == game->player.selected_option ? 100 : 50,
            i == game->player.selected_option ? 100 : 50,
            255);
        
        SDL_Rect option_box = {
            SCREEN_WIDTH / 6 + 20,
            SCREEN_HEIGHT - 120 + (i * 30),
            SCREEN_WIDTH * 2 / 3 - 40,
            25
        };
        SDL_RenderFillRect(game->renderer, &option_box);

        // draw option text
        SDL_SetRenderDrawColor(game->renderer, 
            i == game->player.selected_option ? 255 : 200,
            i == game->player.selected_option ? 255 : 200,
            i == game->player.selected_option ? 255 : 200,
            255);
            
        for (size_t j = 0; j < strlen(current_node.options[i].text); j++) {
            SDL_Rect char_rect = {
                SCREEN_WIDTH / 6 + 30 + (j * char_width),
                SCREEN_HEIGHT - 115 + (i * 30),
                char_width,
                char_height
            };
            SDL_RenderFillRect(game->renderer, &char_rect);
        }
    }

    // draw interaction prompt when not in dialog
    if (!game->player.in_dialog) {
        const char* prompt = "Press E to talk";
        SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
        for (size_t i = 0; i < strlen(prompt); i++) {
            SDL_Rect char_rect = {
                SCREEN_WIDTH / 2 - ((strlen(prompt) * char_width) / 2) + (i * char_width),
                SCREEN_HEIGHT / 2 + 50,
                char_width,
                char_height
            };
            SDL_RenderFillRect(game->renderer, &char_rect);
        }
    }

    // draw current node text
    char debug_text[32];
    sprintf(debug_text, "Node: %d", talking_npc->dialog_tree->current_node);
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255);
    for (size_t i = 0; i < strlen(debug_text); i++) {
        SDL_Rect char_rect = {
            10 + (i * char_width),
            10,
            char_width,
            char_height
        };
        SDL_RenderFillRect(game->renderer, &char_rect);
    }
}

void handle_dialog_input(GameState *game) {
    if (!game->player.in_dialog) return;

    NPC *talking_npc = NULL;
    for (int i = 0; i < MAX_NPCS; i++) {
        if (game->npcs[i].active) {
            float dx = game->npcs[i].x - game->player.x;
            float dy = game->npcs[i].y - game->player.y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 2.0f) {
                talking_npc = &game->npcs[i];
                break;
            }
        }
    }

    if (!talking_npc || !talking_npc->dialog_tree) return;

    DialogNode *current_node = &talking_npc->dialog_tree->nodes[talking_npc->dialog_tree->current_node];

    const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
    static int up_pressed = 0;
    static int down_pressed = 0;
    static int enter_pressed = 0;

    // handle option selection
    if (keyboard_state[SDL_SCANCODE_UP] && !up_pressed) {
        game->player.selected_option = (game->player.selected_option - 1 + current_node->option_count) % current_node->option_count;
        up_pressed = 1;
    } else if (!keyboard_state[SDL_SCANCODE_UP]) {
        up_pressed = 0;
    }

    if (keyboard_state[SDL_SCANCODE_DOWN] && !down_pressed) {
        game->player.selected_option = (game->player.selected_option + 1) % current_node->option_count;
        down_pressed = 1;
    } else if (!keyboard_state[SDL_SCANCODE_DOWN]) {
        down_pressed = 0;
    }

    // handle option selection
    if (keyboard_state[SDL_SCANCODE_RETURN] && !enter_pressed) {
        DialogOption *selected_option = &current_node->options[game->player.selected_option];
        
        // execute option action if there is one
        if (selected_option->action) {
            selected_option->action(game);
        }

        // move to next dialog node
        talking_npc->dialog_tree->current_node = selected_option->next_node;

        // check if we've reached an end node
        if (talking_npc->dialog_tree->nodes[selected_option->next_node].is_end_node) {
            game->player.in_dialog = 0;
            talking_npc->has_been_talked_to = 1;
        }

        enter_pressed = 1;
        game->player.selected_option = 0;
    } else if (!keyboard_state[SDL_SCANCODE_RETURN]) {
        enter_pressed = 0;
    }
}

void handle_input(GameState *game) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            game->is_running = 0;
        }
        else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.scancode == SDL_SCANCODE_E) {
                for (int i = 0; i < MAX_NPCS; i++) {
                    if (game->npcs[i].active) {
                        float dx = game->npcs[i].x - game->player.x;
                        float dy = game->npcs[i].y - game->player.y;
                        float distance = sqrt(dx * dx + dy * dy);
                        if (distance < 2.0f && !game->player.in_dialog) {
                            printf("Starting dialog with %s\n", game->npcs[i].name);
                            game->player.in_dialog = 1;
                            game->player.selected_option = 0;
                            if (!game->npcs[i].has_been_talked_to && game->npcs[i].dialog_tree) {
                                game->npcs[i].dialog_tree->current_node = 0;
                                printf("Reset to initial dialog node\n");
                            }
                            break;
                        }
                    }
                }
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && game->player.in_dialog) {
                game->player.in_dialog = 0;
                printf("Dialog ended\n");
            }
        }
    }

    // handle movement if not in dialog
    if (!game->player.in_dialog) {
        float move_speed = 0.1f;
        float rotation_speed = 0.05f;

        if (game->keyboard_state[SDL_SCANCODE_W]) {
            float new_x = game->player.x + cos(game->player.angle) * move_speed;
            float new_y = game->player.y + sin(game->player.angle) * move_speed;
            if (!world_map[(int)new_y][(int)new_x]) {
                game->player.x = new_x;
                game->player.y = new_y;
            }
        }
        if (game->keyboard_state[SDL_SCANCODE_S]) {
            float new_x = game->player.x - cos(game->player.angle) * move_speed;
            float new_y = game->player.y - sin(game->player.angle) * move_speed;
            if (!world_map[(int)new_y][(int)new_x]) {
                game->player.x = new_x;
                game->player.y = new_y;
            }
        }
        if (game->keyboard_state[SDL_SCANCODE_A]) {
            game->player.angle -= rotation_speed;
        }
        if (game->keyboard_state[SDL_SCANCODE_D]) {
            game->player.angle += rotation_speed;
        }
    } else {
        handle_dialog_input(game);
    }
}


void cast_rays(GameState *game) {
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
    SDL_RenderClear(game->renderer);

    // draw floor
    SDL_SetRenderDrawColor(game->renderer, 40, 40, 40, 255);
    SDL_Rect floor_rect = {0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
    SDL_RenderFillRect(game->renderer, &floor_rect);

    // draw ceiling
    SDL_SetRenderDrawColor(game->renderer, 80, 80, 80, 255);
    SDL_Rect ceiling_rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
    SDL_RenderFillRect(game->renderer, &ceiling_rect);

    float ray_angle = game->player.angle - (FOV / 2) * M_PI / 180;
    
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        float ray_cos = cos(ray_angle);
        float ray_sin = sin(ray_angle);
        float ray_x = game->player.x;
        float ray_y = game->player.y;
        float distance = 0;
        int hit_wall = 0;

        while (!hit_wall && distance < 20) {
            distance += 0.1f;
            ray_x = game->player.x + distance * ray_cos;
            ray_y = game->player.y + distance * ray_sin;

            if (ray_x < 0 || ray_x >= MAP_WIDTH || ray_y < 0 || ray_y >= MAP_HEIGHT) {
                hit_wall = 1;
                distance = 20;
            } else if (world_map[(int)ray_y][(int)ray_x] == 1) {
                hit_wall = 1;
            }
        }

        float ca = game->player.angle - ray_angle;
        distance = distance * cos(ca);

        int line_height = (int)(SCREEN_HEIGHT / distance);
        int draw_start = -line_height / 2 + SCREEN_HEIGHT / 2;
        if (draw_start < 0) draw_start = 0;
        int draw_end = line_height / 2 + SCREEN_HEIGHT / 2;
        if (draw_end >= SCREEN_HEIGHT) draw_end = SCREEN_HEIGHT - 1;

        int color_intensity = (int)(255 - (distance * 10));
        if (color_intensity < 0) color_intensity = 0;
        SDL_SetRenderDrawColor(game->renderer, color_intensity, color_intensity, color_intensity, 255);
        SDL_RenderDrawLine(game->renderer, x, draw_start, x, draw_end);

        ray_angle += (FOV * M_PI / 180) / SCREEN_WIDTH;
    }

    // draw NPCs with interaction indicators
    for (int i = 0; i < MAX_NPCS; i++) {
        if (game->npcs[i].active) {
            float dx = game->npcs[i].x - game->player.x;
            float dy = game->npcs[i].y - game->player.y;
            float distance = sqrt(dx * dx + dy * dy);
            
            float npc_angle = atan2(dy, dx) - game->player.angle;
            
            while (npc_angle < -M_PI) npc_angle += 2 * M_PI;
            while (npc_angle > M_PI) npc_angle -= 2 * M_PI;

            if (fabs(npc_angle) < (FOV * M_PI / 180) / 2) {
                int screen_x = (int)((SCREEN_WIDTH / 2) * (1 + npc_angle / (FOV * M_PI / 180)));
                int npc_height = (int)(SCREEN_HEIGHT / distance);
                int npc_y = SCREEN_HEIGHT / 2 - npc_height / 2;

                // draw NPC
                SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
                SDL_Rect npc_rect = {
                    screen_x - npc_height/4,
                    npc_y,
                    npc_height/2,
                    npc_height
                };
                SDL_RenderFillRect(game->renderer, &npc_rect);

                // interaction indicator if player is close enough
                if (distance < 2.0f) {
                    SDL_SetRenderDrawColor(game->renderer, 255, 255, 0, 255);
                    SDL_Rect indicator = {
                        screen_x - 5,
                        npc_y - 20,
                        10,
                        10
                    };
                    SDL_RenderFillRect(game->renderer, &indicator);
                }
            }
        }
    }

    // dialog if active
    if (game->player.in_dialog) {
        render_dialog_box(game);
    }

    SDL_RenderPresent(game->renderer);
}
int init_game(GameState *game) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }

    game->window = SDL_CreateWindow("RPG Game",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SCREEN_WIDTH,
                                  SCREEN_HEIGHT,
                                  SDL_WINDOW_SHOWN);
    if (!game->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 0;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
    if (!game->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return 0;
    }

    game->is_running = 1;
    game->keyboard_state = SDL_GetKeyboardState(NULL);
    
    init_player(&game->player);
    init_npcs(game->npcs);

    return 1;
}

void render_minimap(GameState *game) {
    int map_size = 100;
    int tile_size = map_size / MAP_WIDTH;
    int padding = 10;
    
    // map background
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 128);
    SDL_Rect map_rect = {padding, padding, map_size, map_size};
    SDL_RenderFillRect(game->renderer, &map_rect);

    // walls
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (world_map[y][x] == 1) {
                SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 128);
                SDL_Rect wall_rect = {
                    padding + x * tile_size,
                    padding + y * tile_size,
                    tile_size,
                    tile_size
                };
                SDL_RenderFillRect(game->renderer, &wall_rect);
            }
        }
    }

    // NPCs on minimap
    for (int i = 0; i < MAX_NPCS; i++) {
        if (game->npcs[i].active) {
            SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
            SDL_Rect npc_rect = {
                padding + (int)(game->npcs[i].x * tile_size) - 2,
                padding + (int)(game->npcs[i].y * tile_size) - 2,
                4,
                4
            };
            SDL_RenderFillRect(game->renderer, &npc_rect);
        }
    }

    // player
    SDL_SetRenderDrawColor(game->renderer, 0, 255, 0, 255);
    SDL_Rect player_rect = {
        padding + (int)(game->player.x * tile_size) - 2,
        padding + (int)(game->player.y * tile_size) - 2,
        4,
        4
    };
    SDL_RenderFillRect(game->renderer, &player_rect);

    // player direction
    int dir_length = 8;
    SDL_RenderDrawLine(game->renderer,
        padding + (int)(game->player.x * tile_size),
        padding + (int)(game->player.y * tile_size),
        padding + (int)(game->player.x * tile_size + cos(game->player.angle) * dir_length),
        padding + (int)(game->player.y * tile_size + sin(game->player.angle) * dir_length)
    );
}

typedef struct Quest {
    int id;
    char title[64];
    char description[256];
    int is_active;
    int is_complete;
    void (*update)(GameState*, struct Quest*);
    NPC* quest_giver;
} Quest;

typedef struct {
    Quest quests[20];
    int quest_count;
} QuestLog;

void update_watchtower_quest(GameState* game, Quest* quest) {
}

void init_quest_log(QuestLog* log) {
    log->quest_count = 0;
}

void add_quest(QuestLog* log, Quest quest) {
    if (log->quest_count < 20) {
        log->quests[log->quest_count++] = quest;
    }
}

void update_quests(GameState* game, QuestLog* log) {
    for (int i = 0; i < log->quest_count; i++) {
        if (log->quests[i].is_active && !log->quests[i].is_complete) {
            if (log->quests[i].update) {
                log->quests[i].update(game, &log->quests[i]);
            }
        }
    }
}
void draw_text(GameState *game, const char *text, int x, int y, SDL_Color color) {
    // use SDL_ttf
    // for now, draw a colored rectangle as a placeholder
    SDL_Rect text_rect = {x, y, strlen(text) * 8, 16};
    SDL_SetRenderDrawColor(game->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(game->renderer, &text_rect);
}

void render_dialog_text(GameState *game, NPC *talking_npc) {
    if (!talking_npc || !talking_npc->dialog_tree) return;

    DialogNode current_node = talking_npc->dialog_tree->nodes[talking_npc->dialog_tree->current_node];
    
    // NPC name
    SDL_Color white = {255, 255, 255, 255};
    draw_text(game, talking_npc->name, SCREEN_WIDTH / 6 + 10, SCREEN_HEIGHT - 300 - 25, white);

    // main dialog text
    draw_text(game, current_node.text, SCREEN_WIDTH / 6 + 20, SCREEN_HEIGHT - 280, white);

    // dialog options
    for (int i = 0; i < current_node.option_count; i++) {
        SDL_Color option_color = {
            i == game->player.selected_option ? 255 : 180,
            i == game->player.selected_option ? 255 : 180,
            i == game->player.selected_option ? 255 : 180,
            255
        };
        draw_text(game, current_node.options[i].text, 
                 SCREEN_WIDTH / 6 + 30,
                 SCREEN_HEIGHT - 120 + (i * 30),
                 option_color);
    }
}

int main(int argc, char *argv[]) {
    GameState game;
    QuestLog quest_log;
    
    if (!init_game(&game)) {
        return 1;
    }

    init_quest_log(&quest_log);

    // main game loop
    while (game.is_running) {
        handle_input(&game);
        cast_rays(&game);
        update_quests(&game, &quest_log);
        render_minimap(&game);
        SDL_Delay(16);
    }

    // cleanup
    for (int i = 0; i < MAX_NPCS; i++) {
        if (game.npcs[i].dialog_tree) {
            free(game.npcs[i].dialog_tree);
        }
    }
    SDL_DestroyRenderer(game.renderer);
    SDL_DestroyWindow(game.window);
    SDL_Quit();

    return 0;
}