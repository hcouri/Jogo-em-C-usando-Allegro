#include <allegro5/allegro5.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <stdbool.h>
#include <stdio.h>
#include "player.h"

#define BLOCK_SIZE 32
#define MAP_WIDTH 13
#define MAP_HEIGHT 10
#define MAX_STACK_SIZE 100

int map[MAP_HEIGHT][MAP_WIDTH];

// Estrutura para os projeteis
typedef struct{
    int x, y;
    int dx, dy;
    bool active;
}Projectile;

typedef struct{
    Projectile stack[MAX_STACK_SIZE];
    int top;
}Stack;

// Função para inicializar a pilha
void initialize_stack(Stack* s){
    s->top = -1;
}

// Função para verificar se a pilha esta vazia
bool is_stack_empty(Stack* s){
    return s->top == -1;
}

// Função para verificar se a pilha esta cheia
bool is_stack_full(Stack* s){
    return s->top == MAX_STACK_SIZE - 1;
}

// Função para adicionar um item a pilha
bool push(Stack* s, Projectile p){
    if (is_stack_full(s)) return false;
    s->stack[++s->top] = p;
    return true;
}

// Função para remover um item da pilha
bool pop(Stack* s, Projectile* p){
    if (is_stack_empty(s)) return false;
    *p = s->stack[s->top--];
    return true;
}

// Função para inicializar o mapa
void initialize_map(const char* filename){
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Falha ao abrir o arquivo '%s'!\n", filename);
        return;
    }

    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            fscanf(file, "%d", &map[i][j]);
        }
    }

    fclose(file);
}

// Função para desenhar o mapa
void draw_map(ALLEGRO_BITMAP* map_image){
    al_draw_bitmap(map_image, 0, 0, 0);
}



// Função para adicionar um projetil
void shoot_projectile(int x, int y, int dx, int dy, Stack* s){
    if (is_stack_full(s)) return;

    Projectile p;
    p.x = x;
    p.y = y;
    p.dx = dx;
    p.dy = dy;
    p.active = true;
    push(s, p);
}

// Função para atualizar os projeteis
bool update_projectiles(int player1_x, int player1_y, Stack* s){
    Stack temp_stack;
    initialize_stack(&temp_stack);
    bool hit = false;

    while (!is_stack_empty(s)) {
        Projectile p;
        pop(s, &p);

        if (p.active) {
            p.x += p.dx;
            p.y += p.dy;

            if (p.x < 0 || p.x > 436 || p.y < 0 || p.y > 336) {
                p.active = false;
            }

            if (check_collision(p.x, p.y, player1_x, player1_y)) {
                hit = true;
            }

            if (p.active) {
                push(&temp_stack, p);
            }
        }
    }

    while (!is_stack_empty(&temp_stack)) {
        Projectile p;
        pop(&temp_stack, &p);
        push(s, p);
    }

    return hit;
}

// Função para desenhar os projeteis
void draw_projectiles(Stack* s){
    Stack temp_stack;
    initialize_stack(&temp_stack);

    while (!is_stack_empty(s)) {
        Projectile p;
        pop(s, &p);

        if (p.active) {
            al_draw_filled_circle(p.x, p.y, 5, al_map_rgb(255, 0, 0)); // Projétil vermelho
            push(&temp_stack, p);
        }
    }

    while (!is_stack_empty(&temp_stack)) {
        Projectile p;
        pop(&temp_stack, &p);
        push(s, p);
    }
}

int main() {
    al_init();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_audio();        // Inicializa o subsistema de audio
    al_init_acodec_addon();    // Inicializa os codecs de audio
    initialize_map("mapa.txt");

    // Reserve canais de áudio
    al_reserve_samples(1);

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0); // 60 FPS
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    // Ajuste o tamanho da janela para corresponder ao tamanho do mapa
    ALLEGRO_DISPLAY* disp = al_create_display(436 , 336);

    // Carrega a trilha sonora
    ALLEGRO_SAMPLE* trilha_sonora = al_load_sample("trilha_sonora.ogg");


    // Toca a trilha sonora em loop
    ALLEGRO_SAMPLE_ID trilha_id;
    al_play_sample(trilha_sonora, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &trilha_id);

    // Carrega a imagem do mapa
    ALLEGRO_BITMAP* map_image = al_load_bitmap("mapa_pacman.png");

    // Carrega a imagem dos sprites
    ALLEGRO_BITMAP* image = al_load_bitmap("sprites.png");

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    bool redraw = true;
    ALLEGRO_EVENT event;

    bool keys[ALLEGRO_KEY_MAX] = {0};

    Stack projectile_stack;
    initialize_stack(&projectile_stack);

    // Posiciona os jogadores em areas andaveis (celulas 0)
    int x1 = 32;  // Jogador 1 em (1, 1)
    int y1 = 32;
    int si1 = 0;
    int flags1 = 0;
    int n1 = 0;

    int x2 = 352; // Jogador 2 em (8, 11)
    int y2 = 256;
    int si2 = 0;
    int flags2 = 0;
    int n2 = 0;

    // Variavel para armazenar o tempo decorrido
    float time_survived = 0;

    al_start_timer(timer);
    while (1) {
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;
            n1++;
            n2++;
            time_survived += 1.0 / 60.0; // Incrementa o tempo em cada quadro

            if (n1 % 12 == 0)
                si1 = (si1 + 1) % 5;
            if (n2 % 12 == 0)
                si2 = (si2 + 1) % 5;

            // Atualiza os projéteis
            if (update_projectiles(x1, y1, &projectile_stack)) {
                // Se um projetil atingiu o jogador 1, encerra o jogo
                printf("GAME OVER! ATINGIDO NO JOELHO!\n");
                break;
            }
        } else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            keys[event.keyboard.keycode] = true;
        } else if (event.type == ALLEGRO_EVENT_KEY_UP) {
            keys[event.keyboard.keycode] = false;
        } else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }

        // Controles do primeiro jogador (setas)
        if (keys[ALLEGRO_KEY_RIGHT])
            move_player(&x1, &y1, 2, 0);
        if (keys[ALLEGRO_KEY_LEFT])
            move_player(&x1, &y1, -2, 0);
        if (keys[ALLEGRO_KEY_DOWN])
            move_player(&x1, &y1, 0, 2);
        if (keys[ALLEGRO_KEY_UP])
            move_player(&x1, &y1, 0, -2);

        // Controles do segundo jogador (WASD)
        if (keys[ALLEGRO_KEY_D])
            move_player(&x2, &y2, 2, 0);
        if (keys[ALLEGRO_KEY_A])
            move_player(&x2, &y2, -2, 0);
        if (keys[ALLEGRO_KEY_S])
            move_player(&x2, &y2, 0, 2);
        if (keys[ALLEGRO_KEY_W])
            move_player(&x2, &y2, 0, -2);

        // Controles para disparar projéteis (por exemplo, com a tecla 'F')
        if (keys[ALLEGRO_KEY_F]) {
            shoot_projectile(x2 + 16, y2 + 16, 2, 0, &projectile_stack); // Dispara o projetil para a direita
            keys[ALLEGRO_KEY_F] = false; // Impede multiplos disparos
        }

        if (keys[ALLEGRO_KEY_ESCAPE])
            break;

        // Verificação de colisao entre os jogadores
        if (check_collision(x1, y1, x2, y2)) {
            printf("GAME OVER! \n");
            break;
        }

        // Verificação de vitoria por tempo
        if (time_survived >= 25.0) {
            printf("PARABENS VOCE VENCEU!!!!!\n");
            break;
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            al_clear_to_color(al_map_rgb(150, 150, 200));
            draw_map(map_image);
            al_draw_bitmap_region(image, 32 * si1, 32 * 3, 32, 32, x1, y1, flags1);
            al_draw_bitmap_region(image, 32 * si2, 32 * 3, 32, 32, x2, y2, flags2);
            draw_projectiles(&projectile_stack); // Desenha os projeteis
            al_flip_display();
            redraw = false;
        }
    }

    al_destroy_bitmap(map_image);
    al_destroy_bitmap(image);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_sample(trilha_sonora); // Libera a memoria da trilha sonora

    return 0;
}
