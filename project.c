#include <locale.h>
#include <ncursesw/ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <wchar.h>
#include <unistd.h>
#include <pthread.h>
//------------
#define MAX_MUSIC_TRACKS 10
#define MAX_ROOMS 6
#define ROOM_MIN_WIDTH 6
#define ROOM_MIN_HEIGHT 6
#define MAX_ATTEMPTS 3 
#define COLOR_ORANGE 16
#define COLOR_yellow 15
#define MAX_FOOD_INVENTORY 5  // حداکثر تعداد غذاهایی که بازیکن می‌تواند حمل کند
#define MAX_GOLD 5
#define MAX_ENEMIES 1
//------------
int move_u=0;
int move_d=0;
int move_r=0;
int move_l=0;

typedef enum {
    DEAMON,
    FIRE_BREATHING_MONSTER,
    GIANT,
    SNAKE,
    UNDEAD
} EnemyType;
typedef struct {
    EnemyType type;       // نوع دشمن
    char symbol;          // نماد نمایشی (D, F, G, S, U)
    int min_damage;       // حداقل آسیب برای نابودی
    int health;           // سلامت (فقط برای SNAKE)
    int chase_steps;      // مراحل باقیمانده برای تعقیب (برای GIANT و UNDEAD)
    bool is_chasing;      // وضعیت تعقیب
    int x, y;             // مختصات فعلی دشمن
} Enemy;
typedef struct {
    Enemy *enemies; // آرایه پویا
    int count;      // تعداد دشمنان فعلی
} EnemyList;
typedef struct {
    int x, y;
    int value;
    int collected;
} GoldBag;
typedef struct {
    int difficulty;
    char main_color[20];
    char music[MAX_MUSIC_TRACKS][50];
    int selected_music;
} Settings;
typedef struct {
    char username[50];
    int total_score;
    int total_gold;
    int games_played;
    time_t first_game_time;
} Player;
typedef struct {
    char name[20];  // نام غذا
    int health_restore;  // مقدار افزایش سلامتی
    int hunger_restore;  // کاهش گرسنگی
    int is_noraml;
    int is_special;
    int is_magic;  // آیا جادویی است؟
    int is_poisoned;  // آیا فاسد است؟
} Food;
typedef struct {
    int x, y, width, height; // موقعیت و اندازه اتاق
    int has_secret_door;     // آیا اتاق دارای در مخفی است؟
    int has_password_door;   // آیا اتاق دارای در رمزدار است؟
    char password[6];        // رمز در صورتی که رمزدار باشد
    int has_master_key;      // آیا کلید خاص در اتاق وجود دارد؟
    int master_key_used;   
    int opend;  // آیا این کلید استفاده شده است؟
    int is_regular;
    int is_treasure;
    int is_enchant;
    int is_nightmare;
    int enemies[5];
    int enemies_move[5];
} Room;
typedef struct {
    Room rooms[MAX_ROOMS];
    int room_count;
    int **map;
    int floor;
} Map;
typedef struct {
    int x ,y;
    char color [20];
    int has_key;
    int has_broken_key;
    int health;
    int inventory[4];  // لیستی از غذاهایی که بازیکن دارد
    int food_count;  // تعداد غذاهای حمل شده
    int hunger;
    int gold;
    int weapon[5];
    int using_weapon;
    int spell[3];
    double play_timer;
    int num_game;
} Hero;
struct timespec start_time, end_time;

int chase_steps_deamon;
int chase_steps_giant;
int chase_steps_fire;
int chase_steps_undeed;
int diraction;
int health_spel=0;
int speed_spel=0;
int damage_spel=0;
int damage_increase=1;
int in_enchant_room;
int win;
int lose;

int **map_check;
int** visible;
int **map_check_floor_2;
int** visible_floor_2;
int **map_check_floor_3;
int** visible_floor_3;
int **map_check_floor_4;
int** visible_floor_4;
int current_floor ;
int ***visible_ptr;
int ***map_check_ptr;
int ***map_check_ptr_floor1;
int ***visible_ptr_floor1;

EnemyList enemies;
GoldBag gold[10];
Settings settings;
Hero hero;
Map map;
Map map_floor_2;
Map map_floor_3;
Map map_floor_4;
Map *map_ptr;
Map *map_ptr_floor1;
bool code_shown = false; // نشان می‌دهد که رمز فعلی نمایش داده شده یا نه
time_t code_start_time = 0;
time_t code_start_time_heal = 0;
time_t code_start_time_hunger = 0;
bool show_full_map = false;
pthread_t timer_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t timer_thread_heal;
pthread_mutex_t mutex_heal = PTHREAD_MUTEX_INITIALIZER;
pthread_t timer_thread_hunger;
pthread_mutex_t mutex_hunger = PTHREAD_MUTEX_INITIALIZER;
//------------// Function declarations
void create_room(Room *room, int max_width, int max_height);
void place_rooms(Map *map, int max_width, int max_height);
void connect_rooms(Map *map);
void add_pillars_stair_traps(Map *map);
void generate_map(Map *map, int max_width, int max_height);
void print_map(Map *map, int max_width, int max_height,char* username);
void print_full_map(Map *map, int max_width, int max_height,char* username);
void display_visible_map(Map*map, int** visible);
void create_user();
void generate_random_password(char *password, int length);
void print_menu(WINDOW *menu_win, int highlight, char **choices, int n_choices);
int start_menu();
void guest_entrance();
int is_username_taken(const char *username);
void user_entrance_menu();
void reset_password_help();
void before_game_menu(char* username);
void start_new_game(char *username);
void continue_game(char *username);
void view_leaderboard(char *username,int j);
void save_matrices(char *username, int** matrices,int i,int j);
void load_matrices(char *username, int** matrices,int i,int j,FILE* file);
double get_elapsed_time(struct timespec start, struct timespec end);
void display_settings_menu(char *username);
void change_difficulty();
void change_color();
void select_music();
void save_settings(const char *username);
void load_settings(char *username);
void save_hero(FILE *file, Hero *hero);
void load_hero(FILE *file, Hero *hero);
void save_room(FILE *file, Room *room);
void save_all_rooms(FILE *file, Map *map);
void load_room(FILE *file, Room *room);
void load_all_rooms(FILE *file, Map *map);
void update_player_score( char *filename,  char *target_username, int score_change, int gold_change);

void secret_room(Map *map,int i);
void password_room(Map *map,int i);
void special_key(Map *map);
void show_code_temporarily(WINDOW *win, int x, int y, char *code);
void* check_code_timer(void* arg);
void* check_code_timer_heal(void* arg);
void* check_code_timer_hunger(void* arg);

/*void load_music(settings);*/
void print_settings_menu(WINDOW *menu_win, int highlight, char **choices, int n_choices);
int hero_movement(Map *map,int***map_check_ptr,int***visible_ptr,char* username);
void move_between_floors(int direction,char*username);
int is_valid_move(Map *map, int new_y, int new_x);
void print_selected_room(Map *map, char* username,int room_num,int** visible);
int which_room(Map *map,int x,int y);
void show_rooms(Map *map,int x,int y);
void kill_music();
void play_music();
void print_colored_massage(char *message, int color_pair);

void add_food_to_hero();
void food_menu();

void enemis_move(Map *map,int***map_check_ptr,int***visible_ptr,char* username,char enemy,int room_num,int i, int j);
void spawn_enemies(EnemyList *list, int map_width, int map_height);
Enemy create_random_enemy(int map_width, int map_height);
/*void consume_food(int index);
void show_food_inventory(hero);*/
//------------
int main() {
    setlocale(LC_CTYPE,"");
    initscr();
    start_color();
    clear();
    noecho();
    cbreak();  // Line buffering disabled. pass on everything
    curs_set(0);
    settings.difficulty=1;
    current_floor =1;
    strcpy(settings.main_color,"Red");
    //strcpy (settings.music,"Track1");
    settings.selected_music=1;
    enemies.enemies = (Enemy*)malloc(MAX_ENEMIES * sizeof(Enemy));
    enemies.count = 0;
    int chase_steps_deamon=5;
    int chase_steps_giant=5;
    int chase_steps_fire=5;
    int chase_steps_undeed=5;
    in_enchant_room=0;
    diraction=0;
    win=0;
    lose=0;
    if (pthread_create(&timer_thread, NULL, check_code_timer, NULL) != 0) {
        perror("Failed to create timer thread");
        return 1;
    }
    if (pthread_create(&timer_thread_heal, NULL, check_code_timer_heal, NULL) != 0) {
        perror("Failed to create timer thread");
        return 1;
    }
    if (pthread_create(&timer_thread_hunger, NULL, check_code_timer_hunger, NULL) != 0) {
        perror("Failed to create timer thread");
        return 1;
    }
    int choice;
    while((choice = start_menu()) != 4) {
        clear();
        switch(choice) {
            case 1:
                clear();
                create_user();
                clear();

                break;
            case 2:
                clear();
                user_entrance_menu();
                break;
            case 3:
                clear();
                guest_entrance();
                break;
            case 4:
                clear();
                endwin();
                break;
            default:
                printw("Invalid choice\n");
                getch();
                break;
        }
    }
    kill_music();
    endwin();
    return 0;
}

void generate_random_password(char *password, int length) {
    const char *digits = "0123456789";
    const char *uppers = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *lowers = "abcdefghijklmnopqrstuvwxyz";
    
    srand(time(NULL)); // Seed the random number generator
    
    password[0] = digits[rand() % strlen(digits)];
    password[1] = uppers[rand() % strlen(uppers)];
    password[2] = lowers[rand() % strlen(lowers)];
    
    for (int i = 3; i < length; ++i) {
        int r = rand() % 3;
        if (r == 0)
            password[i] = digits[rand() % strlen(digits)];
        else if (r == 1)
            password[i] = uppers[rand() % strlen(uppers)];
        else
            password[i] = lowers[rand() % strlen(lowers)];
    }
    password[length] = '\0';
}

int start_menu() {
    WINDOW *menu_win;
    int highlight = 1;
    int choice = 0;
    int c;

    char *choices[] = {
        "New player",
        "Exist player",
        "geust",
        "Exit"
    };
    int n_choices = sizeof(choices) / sizeof(char *);

    menu_win = newwin(10, 20, (LINES/2)-12, (COLS/2)-3);
    keypad(menu_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
    refresh();
    print_menu(menu_win, highlight, choices, n_choices);
    while(1) {
        c = wgetch(menu_win);
        switch(c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10:
                choice = highlight;
                break;
            default:
                refresh();
                break;
        }
        print_menu(menu_win, highlight, choices, n_choices);
        if (choice != 0)
            break;
    }
    clrtoeol();
    refresh();
    delwin(menu_win);
    return choice;
}

void print_menu(WINDOW *menu_win, int highlight, char **choices, int n_choices) {
    int x, y, i;

    x = 5;
    y = 3;
    box(menu_win, 0, 0);
    for(i = 0; i < n_choices; ++i) {
        if(highlight == i + 1) { /* High light the present choice */
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", choices[i]);
            wattroff(menu_win, A_REVERSE);
        } else {
            mvwprintw(menu_win, y, x, "%s", choices[i]);
        }
        ++y;
    }
    wrefresh(menu_win);
}

void create_user() {
    timeout(-1);
    char username[50];
    char password[50];
    char email[50];
    char filename[60];
    FILE *file;

    // Initialize ncurses window for input
    WINDOW *input_win = newwin(12, 60, (LINES - 12) / 2, (COLS - 50) / 2);
    box(input_win, 0, 0);
    refresh();
    wrefresh(input_win);
    keypad(input_win, TRUE);

    // Get username
    mvwprintw(input_win, 1, 1, "Enter Username: ");
    echo();  // Enable echo to see the input
    mvwgetstr(input_win, 1, 17, username);
    noecho();  // Disable echo again

    // Check if username is taken
    if (is_username_taken(username)) {
        mvwprintw(input_win, 7, 1, "Username already taken, please choose another");
        mvwprintw(input_win, 8, 1, "Press enter to refill...");
        wrefresh(input_win);
        getch();
        create_user();
        return;
    }

    // Option to generate random password
    mvwprintw(input_win, 3, 1, "Press 'g' to generate a random password");
    wrefresh(input_win);

    // Get password
    mvwprintw(input_win, 4, 1, "Enter Password: ");
    echo();
    int ch = wgetch(input_win);
    if (ch == 'g') {
        generate_random_password(password, 7);
        mvwprintw(input_win, 5, 1, "Generated Password: %s", password);
        noecho();
        wrefresh(input_win);
    } else {
        mvwgetstr(input_win, 4, 17, password);
        noecho();
    }

    // Get email
    mvwprintw(input_win, 6, 1, "Enter Email: ");
    echo();
    mvwgetstr(input_win, 6, 17, email);
    noecho();

    // Check constraints
    if (strlen(password) < 7) {
        mvwprintw(input_win, 8, 1, "Password must be at least 7 characters long");
        mvwprintw(input_win, 9, 1, "Press enter to refill...");
        wrefresh(input_win);
        getch();
        return;
    }

    int has_digit = 0, has_upper = 0, has_lower = 0;
    for (int i = 0; i < strlen(password); i++) {
        if (isdigit(password[i])) has_digit = 1;
        if (isupper(password[i])) has_upper = 1;
        if (islower(password[i])) has_lower = 1;
    }
    if (!has_digit || !has_upper || !has_lower) {
        mvwprintw(input_win, 8, 1, "Password must contain at least one digit, one upper case letter, and one lower case letter");
        mvwprintw(input_win, 9, 1, "Press enter to refill...");
        wrefresh(input_win);
        getch();
        return;
    }
    int validate_email=0;
    for(int i=0;i<strlen(email);i++){
        if(email[i]=='@'){
            for(int j=i;j<strlen(email);j++){
                if(email[j]=='.'){
                    validate_email=1;
                    break;
                }
            }
        }
        if(validate_email==1){
            break;
        }
    }
    if (validate_email==0) {
        mvwprintw(input_win, 8, 1, "Invalid email format");
        mvwprintw(input_win, 9, 1, "Press enter to refill...");
        wrefresh(input_win);
        getch();
        return;
    }

    snprintf(filename, sizeof(filename), "%s.txt", username);
    FILE* file_user= fopen(filename, "a");
    fprintf(file_user, "Username: %s\nPassword: %s\nEmail: %s\n\nDifficulty: %d\nMain Color: %s\nSelected Music: %d", username, password, email,1,"Red",1);
    fclose(file_user);
    file=fopen("leaderboard.txt","a");
    time_t current_time = time(NULL);
    struct tm *time_info = localtime(&current_time);

    // ذخیره‌سازی زمان در فایل
    time_t now = time(NULL);
    fprintf(file, "%s,%d,%d,%d,%ld\n", username,0,0,0,now);
    fclose(file);
    
    mvwprintw(input_win, 8, 1, "User created successfully!");
    wrefresh(input_win);
    getch();
}

int is_username_taken(const char *username) {
    char filename[60];
    snprintf(filename, sizeof(filename), "%s.txt", username);
    FILE *file = fopen(filename, "r");
    if (!file) return 0; // If file doesn't exist, username is not taken

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "Username: ") == line) { // Check if line starts with "Username: "
            char existing_username[50];
            sscanf(line, "Username: %s", existing_username);
            if (strcmp(existing_username, username) == 0) {
                fclose(file);
                return 1; // Username is taken
            }
        }
    }
    fclose(file);
    return 0;
}
void user_entrance_menu(){
    timeout(-1);
    char username[50];
    char password[50];
    char filename[60];
    char stored_username[50];
    char stored_password[50];
    int authenticated = 0;
    char line[100];

    // Initialize ncurses window for input
    WINDOW *input_win = newwin(12, 60, (LINES - 12) / 2, (COLS - 50) / 2);
    box(input_win, 0, 0);
    refresh();
    wrefresh(input_win);
    keypad(input_win, TRUE);

    // Get username
    mvwprintw(input_win, 1, 1, "Enter Username: ");
    echo();  // Enable echo to see the input
    mvwgetstr(input_win, 1, 17, username);
    noecho();  // Disable echo again

    // Check if username is taken
    if (!is_username_taken(username)) {
        mvwprintw(input_win, 7, 1, "This username doesn't exist!");
        mvwprintw(input_win, 8, 1, "Press enter to refill...");
        wrefresh(input_win);
        getch();
        user_entrance_menu();
    }

    // Option to generate random password
    mvwprintw(input_win, 3, 1, "Press 'f' if you forget your password");
    wrefresh(input_win);

    // Get password
    mvwprintw(input_win, 4, 1, "Enter Password: ");
    echo();
    int ch = wgetch(input_win);
    if (ch == 'f') {
        clear();
        reset_password_help();
        //mvwprintw(input_win, 5, 1, "Generated Password: %s", password);
        noecho();
        clear();
        user_entrance_menu();
        wrefresh(input_win);
    } else {
        mvwgetstr(input_win, 4, 17, password);
        noecho();
    }
    snprintf(filename, sizeof(filename), "%s.txt", username);
    FILE* file= fopen(filename, "r");
    if (file == NULL) {
        mvwprintw(input_win, 5, 1, "Error opening user file");
        wrefresh(input_win);
        getch();
        delwin(input_win);
        return;
    }

    // Check if the username and password are correct
    fscanf(file, "Username: %s\nPassword: %s\n", stored_username, stored_password);
    fclose(file);

    // Check if the username and password are correct
    if (strcmp(username, stored_username) == 0 && strcmp(password, stored_password) == 0) {
        authenticated = 1;
    }

    if (authenticated) {
        mvwprintw(input_win, 5, 1, "Login successful");
        wrefresh(input_win);
        getch();
        delwin(input_win);
        clear();
        before_game_menu(username);
/*
        // Show options for new game or continue game
        WINDOW *menu_win;
        int highlight = 1;
        int choice = 0;
        int c;

        char *choices[] = {
            "Start New Game",
            "Continue Previous Game",
            "View Leaderboard",
            "Exit"
        };
        int n_choices = sizeof(choices) / sizeof(char *);

        menu_win = newwin(10, 40, (LINES - 10) / 2, (COLS - 40) / 2);
        keypad(menu_win, TRUE);
        mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
        refresh();
        print_menu(menu_win, highlight, choices, n_choices);

        while (1) {
            c = wgetch(menu_win);
            switch (c) {
                case KEY_UP:
                    if (highlight == 1)
                        highlight = n_choices;
                    else
                        --highlight;
                    break;
                case KEY_DOWN:
                    if (highlight == n_choices)
                        highlight = 1;
                    else
                        ++highlight;
                    break;
                case 10: // Enter key
                    choice = highlight;
                    break;
                default:
                    refresh();
                    break;
            }
            print_menu(menu_win, highlight, choices, n_choices);
            if (choice != 0)
                break;
        }
        clrtoeol();
        refresh();
        delwin(menu_win);

        // Handle the user's choice
        switch (choice) {
            case 1:
                // Start a new game
                clear();
                start_new_game(username);
                break;
            case 2:
                // Continue the previous game
                clear();
                continue_game(username);
                break;
            case 3:
                //clear();
                view_leaderboard(username);
                break;
            case 4:
                // Exit
                break;
        }
    }*/
    } else {
        mvwprintw(input_win, 5, 1, "Invalid username or password");
        wrefresh(input_win);
        getch();
        delwin(input_win);
        user_entrance_menu();
    }
}
void reset_password_help() {
    char username[50];
    char email[50];
    char filename[60];
    char stored_username[50];
    char stored_email[50];
    char stored_password[50];
    FILE *file;

    // Initialize ncurses window for input
    WINDOW *input_win = newwin(10, 50, (LINES - 10) / 2, (COLS - 50) / 2);
    box(input_win, 0, 0);
    refresh();
    wrefresh(input_win);
    keypad(input_win, TRUE);

    // Get username
    mvwprintw(input_win, 1, 1, "Enter Username: ");
    echo();
    mvwgetstr(input_win, 1, 17, username);
    noecho();
    if(!is_username_taken(username)){
        mvwprintw(input_win, 7, 1, "This username doesn't exist!");
        mvwprintw(input_win, 8, 1, "Press enter to refill...");
        wrefresh(input_win);
        getch();
        reset_password_help();    
    }
    // Get email
    mvwprintw(input_win, 3, 1, "Enter Email: ");
    echo();
    mvwgetstr(input_win, 3, 17, email);
    noecho();

    // Create the filename based on the username
    snprintf(filename, sizeof(filename), "%s.txt", username);

    // Open the user's file for reading
    file = fopen(filename, "r");
    if (file == NULL) {
        mvwprintw(input_win, 5, 1, "Error opening user file");
        wrefresh(input_win);
        getch();
        delwin(input_win);
        return;
    }

    // Read the stored username and email from the file
    fscanf(file, "Username: %s\nPassword: %s\nEmail: %s\n", stored_username,stored_password, stored_email);
    fclose(file);

    // Check if the username and email are correct
    if (strcmp(username, stored_username) == 0 && strcmp(email, stored_email) == 0) {
        mvwprintw(input_win, 5, 1, "Your entered email is correct.");
        mvwprintw(input_win, 6, 1, "Your password is: %s",stored_password);
    } else {
        mvwprintw(input_win, 5, 1, "Invalid email. Please try later.");
        //getch();
        //user_entrance_menu();
    }
    wrefresh(input_win);
    getch();
    delwin(input_win);
}
void guest_entrance(){
    timeout(-1);
    WINDOW *menu_win;
    int highlight = 1;
    int choice = 0;
    int c;

    char *choices[] = {
        "Start New Game",
        "Exit"
    };
    int n_choices = sizeof(choices) / sizeof(char *);

    menu_win = newwin(10, 40, (LINES - 10) / 2, (COLS - 40) / 2);
    keypad(menu_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
    refresh();
    print_menu(menu_win, highlight, choices, n_choices);

    while (1) {
        c = wgetch(menu_win);
        switch (c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10: // Enter key
                choice = highlight;
                break;
            default:
                refresh();
                break;
        }
        print_menu(menu_win, highlight, choices, n_choices);
        if (choice != 0)
            break;
    }
    clrtoeol();
    refresh();
    delwin(menu_win);
    char username[10]="guest";
    // Handle the user's choice
    switch (choice) {
        case 1:
            // Start a new game
            clear();
            start_new_game(username);
            break;       
        case 2:
            // Exit
            clear();
            start_menu();
            break;
    }    
}
void before_game_menu(char* username){
/*    mvwprintw(input_win, 5, 1, "Login successful");
    wrefresh(input_win);
    getch();
    delwin(input_win);
*/
    // Show options for new game or continue game
    timeout(-1);
    WINDOW *menu_win;
    int highlight = 1;
    int choice = 0;
    int c;

    char *choices[] = {
        "Start New Game",
        "Continue Previous Game",
        "View Leaderboard",
        "Settings",
        "Exit"
    };
    int n_choices = sizeof(choices) / sizeof(char *);

    menu_win = newwin(10, 40, (LINES - 10) / 2, (COLS - 40) / 2);
    keypad(menu_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
    refresh();
    print_menu(menu_win, highlight, choices, n_choices);

    while (1) {
        c = wgetch(menu_win);
        switch (c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10: // Enter key
                choice = highlight;
                break;
            default:
                refresh();
                break;
        }
        print_menu(menu_win, highlight, choices, n_choices);
        if (choice != 0)
            break;
    }
    clrtoeol();
    refresh();
    delwin(menu_win);

    // Handle the user's choice
    switch (choice) {
        case 1:
            // Start a new game
            clear();
            start_new_game(username);
            break;
        case 2:
            // Continue the previous game
            clear();
            continue_game(username);
            break;
        case 3:
            //clear();
            view_leaderboard(username,0);
            clear();
            before_game_menu(username);
            break;
        case 4:
            // Exit
            clear();
            display_settings_menu(username);
            clear();
            before_game_menu(username);
            break;        
        case 5:
            // Exit
            clear();
            start_menu();
            break;
    }
}
void start_new_game(char *username) {
    hero.food_count=0;
    hero.health=10;
    hero.hunger=0;
    hero.inventory[0]=0;
    hero.inventory[1]=0;
    hero.inventory[2]=0;
    hero.inventory[3]=0;
    hero.weapon[0]=0;
    hero.weapon[1]=0;
    hero.weapon[2]=0;
    hero.weapon[3]=0;
    hero.weapon[4]=0;
    hero.using_weapon=0;
    hero.spell[0]=0;
    hero.spell[1]=0;
    hero.spell[2]=0;
    // Save the initial game state
    int max_width, max_height;
    getmaxyx(stdscr, max_height, max_width);
    map.map = (int **)malloc(max_height * sizeof(int *));
    map_check=(int **)malloc(max_height * sizeof(int *));
    visible=(int **)malloc(max_height * sizeof(int *));

    map_floor_2.map = (int **)malloc(max_height * sizeof(int *));
    map_check_floor_2=(int **)malloc(max_height * sizeof(int *));
    visible_floor_2=(int **)malloc(max_height * sizeof(int *));

    map_floor_3.map = (int **)malloc(max_height * sizeof(int *));
    map_check_floor_3=(int **)malloc(max_height * sizeof(int *));
    visible_floor_3=(int **)malloc(max_height * sizeof(int *));

    map_floor_4.map = (int **)malloc(max_height * sizeof(int *));
    map_check_floor_4=(int **)malloc(max_height * sizeof(int *));
    visible_floor_4=(int **)malloc(max_height * sizeof(int *));

    map.floor=1;
    map_floor_2.floor=2;
    map_floor_3.floor=3;
    map_floor_4.floor=4;
    for (int i = 0; i < max_height; i++) {
        map.map[i] = (int *)malloc(max_width * sizeof(int));
        map_check[i] = (int *)malloc(max_width * sizeof(int));
        visible[i] = (int *)malloc(max_width * sizeof(int));

        map_floor_2.map[i] = (int *)malloc(max_width * sizeof(int));
        map_check_floor_2[i] = (int *)malloc(max_width * sizeof(int));
        visible_floor_2[i] = (int *)malloc(max_width * sizeof(int));

        map_floor_3.map[i] = (int *)malloc(max_width * sizeof(int));
        map_check_floor_3[i] = (int *)malloc(max_width * sizeof(int));
        visible_floor_3[i] = (int *)malloc(max_width * sizeof(int));

        map_floor_4.map[i] = (int *)malloc(max_width * sizeof(int));
        map_check_floor_4[i] = (int *)malloc(max_width * sizeof(int));
        visible_floor_4[i] = (int *)malloc(max_width * sizeof(int));
    }
    srand(time(NULL));
    generate_map(&map, max_width, max_height);
    generate_map(&map_floor_2, max_width, max_height);
    generate_map(&map_floor_3, max_width, max_height);
    generate_map(&map_floor_4, max_width, max_height);
    for (int y = 0; y < max_height; y++) {
        for (int x = 0; x < max_width; x++) {
            map_check[y][x]=map.map[y][x];
            visible[y][x]=0;

            map_check_floor_2[y][x]=map_floor_2.map[y][x];
            visible_floor_2[y][x]=0;

            map_check_floor_3[y][x]=map_floor_3.map[y][x];
            visible_floor_3[y][x]=0;

            map_check_floor_4[y][x]=map_floor_4.map[y][x];
            visible_floor_4[y][x]=0;
        }
    }
    print_map(&map, max_width, max_height,username);
    play_music();
    load_settings(username);
    char quit;

    map_ptr=&map;
    map_ptr_floor1=&map;

    map_check_ptr=&map_check;
    map_check_ptr_floor1=&map_check;

    visible_ptr=&visible;
    visible_ptr_floor1=&visible;
    double elapsed_time; // متغیر برای ذخیره زمان بازی
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    timeout(10);
    while (1) {
        if(hero.health==0){
            lose=1;
            break;
        }
        mvprintw(0, 39, "                         ");
        mvprintw(0, 40, "Your health:");
        for(int i=0 ;i< hero.health;i++){
            mvprintw(0, 54+i, "#");
        }
        if (code_shown && difftime(time(NULL), code_start_time) >= 30) {
                mvprintw(0,COLS-10, "              "); // پاک کردن پیام
                code_shown = false; // غیرفعال کردن نمایش رمز
        }
        if (difftime(time(NULL), code_start_time_heal) >= 10) {
            if(hero.health<10){
                hero.health++;
                if(health_spel>0){
                    hero.health++;
                }
            } // غیرفعال کردن نمایش رمز
            //mvprintw(1, 0, "Your health: %d              ",hero.health);// غیرفعال کردن نمایش رمز
            pthread_mutex_lock(&mutex_heal);
            code_start_time_heal = time(NULL); // زمان شروع نمایش رمز
            pthread_mutex_unlock(&mutex_heal);
        }
        if (difftime(time(NULL), code_start_time_hunger) >= 5) {
            if(hero.hunger<10){
                hero.hunger++;
            } 
            else{
                hero.health--;
            } 
            pthread_mutex_lock(&mutex_heal);
            code_start_time_hunger = time(NULL); // زمان شروع نمایش رمز
            pthread_mutex_unlock(&mutex_heal);
        }
        if(damage_spel>0 && (quit=='1'||quit=='2'||quit=='3'||quit=='4'||quit=='5'||quit=='6'||quit=='7'||quit=='8')){
            damage_spel-=1;
        }
        if(health_spel>0 && (quit=='1'||quit=='2'||quit=='3'||quit=='4'||quit=='5'||quit=='6'||quit=='7'||quit=='8')){
            health_spel-=1;
        }
        if(speed_spel>0 && (quit=='1'||quit=='2'||quit=='3'||quit=='4'||quit=='5'||quit=='6'||quit=='7'||quit=='8')){
            speed_spel-=1;
        }
        if((quit=hero_movement(map_ptr,map_check_ptr,visible_ptr,username))=='q'){
            break;
        }
        refresh();
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    elapsed_time = get_elapsed_time(start_time, end_time);
    hero.play_timer=elapsed_time;
    printf("elapsed time : %.2f sec\n", elapsed_time);

    char filename[60];
    snprintf(filename, sizeof(filename), "%s_game.txt", username);
    FILE *file = fopen(filename, "w");
    fclose(file);
    save_matrices(username,map.map,max_height,max_width);
    save_matrices(username,map_check,max_height,max_width);
    save_matrices(username,visible,max_height,max_width);

    save_matrices(username,map_floor_2.map,max_height,max_width);
    save_matrices(username,map_check_floor_2,max_height,max_width);
    save_matrices(username,visible_floor_2,max_height,max_width);

    save_matrices(username,map_floor_3.map,max_height,max_width);
    save_matrices(username,map_check_floor_3,max_height,max_width);
    save_matrices(username,visible_floor_3,max_height,max_width);

    save_matrices(username,map_floor_4.map,max_height,max_width);
    save_matrices(username,map_check_floor_4,max_height,max_width);
    save_matrices(username,visible_floor_4,max_height,max_width);
    update_player_score("leaderboard.txt",username,hero.gold,hero.gold);
    char filename_h[60];
    snprintf(filename_h, sizeof(filename_h), "%s_hero.txt", username);    
    FILE *file_player = fopen(filename_h, "w");
    save_hero(file_player,&hero);
    save_all_rooms(file_player,&map);
    save_all_rooms(file_player,&map_floor_2);
    save_all_rooms(file_player,&map_floor_3);
    save_all_rooms(file_player,&map_floor_4);
    fclose(file_player);

    for (int i = 0; i < max_height; i++) {
        free(map.map[i]);
        free(map_floor_2.map[i]);
        free(map_floor_3.map[i]);
        free(map_floor_4.map[i]);
        free(map_check[i]);
        free(map_check_floor_2[i]);
        free(map_check_floor_3[i]);
        free(map_check_floor_4[i]);
        free(visible[i]);
        free(visible_floor_2[i]);
        free(visible_floor_3[i]);
        free(visible_floor_4[i]);
    }
    free(map.map);
    free(map_floor_2.map);
    free(map_floor_3.map);
    free(map_floor_4.map);
    free(map_check);
    free(map_check_floor_2);
    free(map_check_floor_3);
    free(map_check_floor_4);
    free(visible);
    free(visible_floor_2);
    free(visible_floor_3);
    free(visible_floor_4);
    before_game_menu(username);
    refresh();
}
void continue_game(char *username) {
    // Load the previous game state from the user's file
    int max_width, max_height;
    getmaxyx(stdscr, max_height, max_width);
    map.map = (int **)malloc(max_height * sizeof(int *));
    map_check=(int **)malloc(max_height * sizeof(int *));
    visible=(int **)malloc(max_height * sizeof(int *));

    map_floor_2.map = (int **)malloc(max_height * sizeof(int *));
    map_check_floor_2=(int **)malloc(max_height * sizeof(int *));
    visible_floor_2=(int **)malloc(max_height * sizeof(int *));

    map_floor_3.map = (int **)malloc(max_height * sizeof(int *));
    map_check_floor_3=(int **)malloc(max_height * sizeof(int *));
    visible_floor_3=(int **)malloc(max_height * sizeof(int *));

    map_floor_4.map = (int **)malloc(max_height * sizeof(int *));
    map_check_floor_4=(int **)malloc(max_height * sizeof(int *));
    visible_floor_4=(int **)malloc(max_height * sizeof(int *));

    map.floor=1;
    map_floor_2.floor=2;
    map_floor_3.floor=3;
    map_floor_4.floor=4;
    for (int i = 0; i < max_height; i++) {
        map.map[i] = (int *)malloc(max_width * sizeof(int));
        map_check[i] = (int *)malloc(max_width * sizeof(int));
        visible[i] = (int *)malloc(max_width * sizeof(int));

        map_floor_2.map[i] = (int *)malloc(max_width * sizeof(int));
        map_check_floor_2[i] = (int *)malloc(max_width * sizeof(int));
        visible_floor_2[i] = (int *)malloc(max_width * sizeof(int));

        map_floor_3.map[i] = (int *)malloc(max_width * sizeof(int));
        map_check_floor_3[i] = (int *)malloc(max_width * sizeof(int));
        visible_floor_3[i] = (int *)malloc(max_width * sizeof(int));

        map_floor_4.map[i] = (int *)malloc(max_width * sizeof(int));
        map_check_floor_4[i] = (int *)malloc(max_width * sizeof(int));
        visible_floor_4[i] = (int *)malloc(max_width * sizeof(int));
    }
    char filename_a[60];
    snprintf(filename_a, sizeof(filename_a), "%s_game.txt", username);
    FILE *file_a = fopen(filename_a, "r");

    if (!file_a) {
        perror("EROR...");
        return;
    }
    load_matrices(username,map.map,max_height,max_width,file_a);
    load_matrices(username,map_check,max_height,max_width,file_a);
    load_matrices(username,visible,max_height,max_width,file_a);

    load_matrices(username,map_floor_2.map,max_height,max_width,file_a);
    load_matrices(username,map_check_floor_2,max_height,max_width,file_a);
    load_matrices(username,visible_floor_2,max_height,max_width,file_a);

    load_matrices(username,map_floor_3.map,max_height,max_width,file_a);
    load_matrices(username,map_check_floor_3,max_height,max_width,file_a);
    load_matrices(username,visible_floor_3,max_height,max_width,file_a);

    load_matrices(username,map_floor_4.map,max_height,max_width,file_a);
    load_matrices(username,map_check_floor_4,max_height,max_width,file_a);
    load_matrices(username,visible_floor_4,max_height,max_width,file_a);
    fclose(file_a);

    char filename_h[60];
    snprintf(filename_h, sizeof(filename_h), "%s_hero.txt", username);    
    FILE *file_player = fopen(filename_h, "r");
    load_hero(file_player,&hero);
    load_all_rooms(file_player,&map);
    load_all_rooms(file_player,&map_floor_2);
    load_all_rooms(file_player,&map_floor_3);
    load_all_rooms(file_player,&map_floor_4);
    fclose(file_player);

    srand(time(NULL));
    switch (current_floor){
        case 1:
            display_visible_map(&map,visible);
            visible_ptr=&visible;
            map_check_ptr=&map_check;
            map_ptr=&map;
            break;
        case 2:
            display_visible_map(&map_floor_2,visible_floor_2);
            visible_ptr=&visible_floor_2;
            map_check_ptr=&map_check_floor_2;
            map_ptr=&map_floor_2;
            break;
        case 3:
            display_visible_map(&map_floor_3,visible_floor_3);
            visible_ptr=&visible_floor_3;
            map_check_ptr=&map_check_floor_3;
            map_ptr=&map_floor_3;
            break;
        case 4:
            display_visible_map(&map_floor_4,visible_floor_4);
            visible_ptr=&visible_floor_4;
            map_check_ptr=&map_check_floor_4;
            map_ptr=&map_floor_4;
            break;
        default:
            break;
    }
    play_music();
    load_settings(username);
    char quit;
    timeout(10);

    map_ptr_floor1=&map;
    map_check_ptr_floor1=&map_check;
    visible_ptr_floor1=&visible;
    while (1) {
        mvprintw(2,0,"%s",settings.main_color);
        //clear();
        if (code_shown && difftime(time(NULL), code_start_time) >= 30) {
                mvprintw(0,COLS-10, "              "); // پاک کردن پیام
                code_shown = false; // غیرفعال کردن نمایش رمز
        }
        if (difftime(time(NULL), code_start_time_heal) >= 10) {
            if(hero.health<10){
                hero.health++;
                if(health_spel>0){
                    hero.health++;
                }
            } // غیرفعال کردن نمایش رمز
            //mvprintw(1, 0, "Your health: %d              ",hero.health);
            pthread_mutex_lock(&mutex_heal);
            code_start_time_heal = time(NULL); // زمان شروع نمایش رمز
            pthread_mutex_unlock(&mutex_heal);
        }
        if (difftime(time(NULL), code_start_time_hunger) >= 5) {
            if(hero.hunger<5){
                hero.hunger++;
            } 
            else{
                hero.health--;
            } // غیرفعال کردن نمایش رمز
            if(in_enchant_room==1){
                hero.health--;
            }
            pthread_mutex_lock(&mutex_heal);
            code_start_time_hunger = time(NULL); // زمان شروع نمایش رمز
          
            pthread_mutex_unlock(&mutex_heal);
        }
        if(damage_spel>0){
            damage_spel-=1;
        }
        if(health_spel>0){
            health_spel-=1;
        }
        if(speed_spel>0){
            speed_spel-=1;
        }
        if((quit=hero_movement(map_ptr,map_check_ptr,visible_ptr,username))=='q'){
            break;
        }
        refresh();
    }
    clear();
    refresh();
    char filename[60];
    snprintf(filename, sizeof(filename), "%s_game.txt", username);
    FILE *file = fopen(filename, "w");
    fclose(file);

    save_matrices(username,map.map,max_height,max_width);
    save_matrices(username,map_check,max_height,max_width);
    save_matrices(username,visible,max_height,max_width);

    save_matrices(username,map_floor_2.map,max_height,max_width);
    save_matrices(username,map_check_floor_2,max_height,max_width);
    save_matrices(username,visible_floor_2,max_height,max_width);

    save_matrices(username,map_floor_3.map,max_height,max_width);
    save_matrices(username,map_check_floor_3,max_height,max_width);
    save_matrices(username,visible_floor_3,max_height,max_width);

    save_matrices(username,map_floor_4.map,max_height,max_width);
    save_matrices(username,map_check_floor_4,max_height,max_width);
    save_matrices(username,visible_floor_4,max_height,max_width);
    update_player_score("leaderboard.txt",username,hero.gold,hero.gold);
    file_player = fopen(filename_h, "w");
    save_hero(file_player,&hero);
    save_all_rooms(file_player,&map);
    save_all_rooms(file_player,&map_floor_2);
    save_all_rooms(file_player,&map_floor_3);
    save_all_rooms(file_player,&map_floor_4);
    fclose(file_player);
    for (int i = 0; i < max_height; i++) {
        free(map.map[i]);
        free(map_floor_2.map[i]);
        free(map_floor_3.map[i]);
        free(map_floor_4.map[i]);
        free(map_check[i]);
        free(map_check_floor_2[i]);
        free(map_check_floor_3[i]);
        free(map_check_floor_4[i]);
        free(visible[i]);
        free(visible_floor_2[i]);
        free(visible_floor_3[i]);
        free(visible_floor_4[i]);
    }
    free(map.map);
    free(map_floor_2.map);
    free(map_floor_3.map);
    free(map_floor_4.map);
    free(map_check);
    free(map_check_floor_2);
    free(map_check_floor_3);
    free(map_check_floor_4);
    free(visible);
    free(visible_floor_2);
    free(visible_floor_3);
    free(visible_floor_4);
    before_game_menu(username);
    refresh();
}

void view_leaderboard(char *username, int j) {
    FILE *file;
    char line[256];
    Player players[100];  // فرض می‌کنیم حداکثر ۱۰۰ کاربر داریم
    int player_count = 0;

    // باز کردن فایل و خواندن اطلاعات کاربران
    file = fopen("leaderboard.txt", "r");
    if (file == NULL) {
        printw("Error opening leaderboard file");
        return;
    }

    // خواندن اطلاعات کاربران از فایل
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%d,%d,%d,%ld", players[player_count].username, 
                                            &players[player_count].total_score, 
                                            &players[player_count].total_gold, 
                                            &players[player_count].games_played, 
                                            &players[player_count].first_game_time);
        player_count++;
    }
    fclose(file);

    // مرتب‌سازی کاربران بر اساس امتیاز کل
    for (int i = 0; i < player_count - 1; i++) {
        for (int k = i + 1; k < player_count; k++) {
            if (players[i].total_score < players[k].total_score) {
                Player temp = players[i];
                players[i] = players[k];
                players[k] = temp;
            }
        }
    }

    // ایجاد پنجره جدول امتیازات
    WINDOW *leaderboard_win = newwin(10, 82, (LINES - 10) / 2, (COLS - 82) / 2);
    box(leaderboard_win, 0, 0);

    int quit = 0;
    while (!quit) {
        werase(leaderboard_win); // پاک کردن محتوای قبلی
        box(leaderboard_win, 0, 0);
        mvwprintw(leaderboard_win, 1, 2, " Rank   Username   Total Score   Total Gold   Games Played   Time Elapsed");
        mvwprintw(leaderboard_win, 2, 1, " ------------------------------------------------------------------------");

        int show_num = 0;
        for (int i = j; i < player_count && show_num < 5; i++) {
            show_num++;
            time_t current_time = time(NULL);
            double time_elapsed = difftime(current_time, players[i].first_game_time) / (60*60);
            init_pair(3, COLOR_BLUE, COLOR_BLACK);
            init_pair(2, COLOR_BLUE, COLOR_BLACK);
            if (strcmp(username, players[i].username) == 0) {
                wattron(leaderboard_win, A_BOLD);
                wattron(leaderboard_win, COLOR_PAIR(3));
            }
            else if (i>=0&&i<=2) {
                wattron(leaderboard_win, A_ITALIC);
                wattron(leaderboard_win, COLOR_PAIR(2));
            }
            const char* medal = "";
            if (i == 0) medal = "🥇";
            else if (i == 1) medal = "🥈";
            else if (i == 2) medal = "🥉";
            if (i>=0&&i<=2) {
                char user[100];
                snprintf(user, sizeof(user), "%s (LEGEND)",players[i].username);
                mvwprintw(leaderboard_win, i - j + 3, 2, "%-5d  %-10s  %-12d  %-10d  %-13d  %.1f hours %s", 
                            i + 1, user , players[i].total_score, 
                            players[i].total_gold, players[i].games_played, time_elapsed, medal);
            }
            else{
                mvwprintw(leaderboard_win, i - j + 3, 2, "%-5d  %-10s  %-12d  %-10d  %-13d  %.1f hours %s", 
                            i + 1, players[i].username, players[i].total_score, 
                            players[i].total_gold, players[i].games_played, time_elapsed, medal);
            }
            if (strcmp(username, players[i].username) == 0) {
                wattroff(leaderboard_win, A_BOLD);
                wattroff(leaderboard_win, COLOR_PAIR(3));
            }
            else if (i>=0&&i<=2) {
                wattroff(leaderboard_win, A_ITALIC);
                wattroff(leaderboard_win, COLOR_PAIR(2));
            }
        }

        wrefresh(leaderboard_win);
        timeout(-1);

        // کنترل حرکت در جدول
        int scroll = getch();
        switch (scroll) {
            case 'u':
                if (j > 0) {
                    j--;  // مقدار را کاهش بده
                }
                break;
            case 'd':
                if (j < player_count - 5) {
                    mvprintw(1,0,"%d",player_count);
                    j++;  // مقدار را افزایش بده
                }
                break;
            case 'q':  // خروج با زدن کلید q
                quit = 1;
                break;
        }
    }

    clrtoeol();
    refresh();
    delwin(leaderboard_win);
}

void display_settings_menu(char *username) {
    WINDOW *settings_win;
    int highlight = 1;
    int choice = 0;
    int c;
    char *choices[] = {
        "Change Difficulty",
        "Change Main Color",
        "Select Music",
        "Save and Exit"
    };
    int n_choices = sizeof(choices) / sizeof(char *);

    settings_win = newwin(10, 40, (LINES/2)-2, (COLS/2)-19);
    keypad(settings_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
    refresh();
    print_settings_menu(settings_win, highlight, choices, n_choices);
    while(1) {
        c = wgetch(settings_win);
        switch(c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10:
                choice = highlight;
                break;
            default:
                refresh();
                break;
        }
        print_settings_menu(settings_win, highlight, choices, n_choices);
        if (choice != 0)
            break;
    }

    switch(choice) {
        case 1:
            clear();
            change_difficulty();
            save_settings(username);
            break;
        case 2:
            clear();
            change_color();
            save_settings(username);
            break;
        case 3:
            clear();
            select_music();
            save_settings(username);
            break;
        case 4:
            save_settings(username);
            return;
        default:
            printw("Invalid choice. Please try again.\n");
            getch();
            break;
    }
}

void change_difficulty() {
    WINDOW *difficulty_win;
    int highlight = 1;
    int choice = 0;
    int c;

    char *difficulties[] = {
        "1",
        "2",
        "3"
    };
    int n_difficulties = sizeof(difficulties) / sizeof(char *);

    difficulty_win = newwin(10, 40, (LINES/2)-2, (COLS/2)-3);
    keypad(difficulty_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
    refresh();
    while(1) {
        for (int i = 0; i < n_difficulties; ++i) {
            if (highlight == i + 1) {
                wattron(difficulty_win, A_REVERSE);
                mvwprintw(difficulty_win, i + 2, 2, "%s", difficulties[i]);
                wattroff(difficulty_win, A_REVERSE);
            } else {
                mvwprintw(difficulty_win, i + 2, 2, "%s", difficulties[i]);
            }
        }
        wrefresh(difficulty_win);

        c = wgetch(difficulty_win);
        switch(c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_difficulties;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == n_difficulties)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10:
                choice = highlight;
                break;
            default:
                refresh();
                break;
        }

        if (choice != 0) {
            settings.difficulty = choice;
            mvprintw(1, 1, "Difficulty %d selected!", settings.difficulty);
            refresh();
            getch();
            break;
        }
    }
    delwin(difficulty_win);
}

void change_color() {
    WINDOW *color_win;
    int highlight = 1;
    int choice = 0;
    int c;

    char *colors[] = {
        "Red",
        "Green",
        "Blue",
        "Yellow"
    };
    int n_colors = sizeof(colors) / sizeof(char *);

    color_win = newwin(10, 40, (LINES/2)-12, (COLS/2)-3);
    keypad(color_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
    refresh();
    while(1) {
        for (int i = 0; i < n_colors; ++i) {
            if (highlight == i + 1) {
                wattron(color_win, A_REVERSE);
                mvwprintw(color_win, i + 2, 2, "%s", colors[i]);
                wattroff(color_win, A_REVERSE);
            } else {
                mvwprintw(color_win, i + 2, 2, "%s", colors[i]);
            }
        }
        wrefresh(color_win);

        c = wgetch(color_win);
        switch(c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_colors;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == n_colors)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10:
                choice = highlight;
                break;
            default:
                refresh();
                break;
        }

        if (choice != 0) {
            strcpy(settings.main_color, colors[choice - 1]);
            mvprintw(1, 1, "Color %s selected!", settings.main_color);
            refresh();
            getch();
            break;
        }
    }

    delwin(color_win);
}


void select_music() {
    WINDOW *music_win;
    int highlight = 1;
    int choice = 0;
    int c;
    char *music[] = {
        "Music-off",
        "Track 1",
        "Track 2",
        "Track 3"
    };
    music_win = newwin(10, 40, (LINES/2)-2, (COLS/2)-3);
    int n_music = sizeof(music) / sizeof(char *);
    keypad(music_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
    refresh();
    while(1) {
        for (int i = 0; i < n_music; ++i) {
            if (highlight == i + 1) {
                wattron(music_win, A_REVERSE);
                mvwprintw(music_win, i + 2, 2, "%s", music[i]);
                wattroff(music_win, A_REVERSE);
            } else {
                mvwprintw(music_win, i + 2, 2, "%s", music[i]);
            }
        }
        wrefresh(music_win);

        c = wgetch(music_win);
        switch(c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_music;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == n_music)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10:
                choice = highlight;
                break;
            default:
                refresh();
                break;
        }
        if (choice != 0) {
            settings.selected_music = choice - 1; // Convert to 0-based index
            mvprintw(1, 1, "Music %d selected!", settings.selected_music);
            refresh();
            getch();
            break;
        }
    }
    delwin(music_win);

}
void save_settings(const char *username) {
    char filename[60];
    snprintf(filename, sizeof(filename), "%s.txt", username);
    FILE *file = fopen(filename, "r+");
    if (file == NULL) {
        printw("Error saving settings.\n");
        return;
    }
    long pos = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "Difficulty") != NULL) {
            pos = ftell(file) - strlen(line);
            break;
        }
    }

    fseek(file, pos, SEEK_SET); // حرکت به موقعیت پیدا شده
    fprintf(file, "Difficulty: %d\n", settings.difficulty);
    fprintf(file, "Main Color: %s\n", settings.main_color);
    fprintf(file, "Selected Music: %d\n", settings.selected_music);
    fclose(file);
}

void load_settings(char *username) {
    char filename[60];
    snprintf(filename, sizeof(filename), "%s.txt", username);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        // Default settings
        settings.difficulty = 1;
        strcpy(settings.main_color, "Red");
        settings.selected_music = 0;
        //load_music(settings);
        return;
    }
    long pos = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "Difficulty") != NULL) {
            pos = ftell(file) - strlen(line);
            break;
        }
    }
    fseek(file, pos, SEEK_SET);
    fscanf(file, "Difficulty: %d\n", &settings.difficulty);
    fscanf(file, "Main Color: %s\n", settings.main_color);
    fscanf(file, "Selected Music: %s\n", settings.music[settings.selected_music]);
    fclose(file);
}

/*void load_music(settings) {
    strcpy(settings.music[0], "Track 1");
    strcpy(settings.music[1], "Track 2");
    strcpy(settings.music[2], "Track 3");
    // Add more tracks as needed

}*/
void print_settings_menu(WINDOW *menu_win, int highlight, char **choices, int n_choices) {
    int x, y, i;

    x = 2;
    y = 2;
    box(menu_win, 0, 0);
    for (i = 0; i < n_choices; ++i) {
        if (highlight == i + 1) {
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", choices[i]);
            wattroff(menu_win, A_REVERSE);
        } else {
            mvwprintw(menu_win, y, x, "%s", choices[i]);
        }
        ++y;
    }
    wrefresh(menu_win);
}
void create_room(Room *room, int max_width, int max_height) {
    room->width = ROOM_MIN_WIDTH + rand() % 3; // حداقل عرض 4، حداکثر 6
    room->height = ROOM_MIN_HEIGHT + rand() % 3; // حداقل ارتفاع 4، حداکثر 6
    room->x = rand() % (max_width - room->width - 1) + 1;
    room->y = rand() % (max_height - room->height - 2) + 2;
    if (rand() % 3 == 0) { // 25% احتمال برای در مخفی
        room->has_secret_door = 1;
    }
}

void place_rooms(Map *map, int max_width, int max_height) {
    map->room_count = 0;
    while (map->room_count < MAX_ROOMS) {
        Room new_room;
        new_room.has_password_door=0;
        new_room.has_secret_door=0;
        if(map->room_count==0){
            new_room.width = 6; // حداقل عرض 4، حداکثر 6
            new_room.height = 6; // حداقل ارتفاع 4، حداکثر 6
            new_room.x = 1;
            new_room.y = 1;  
            map->rooms[map->room_count++] = new_room;
            for (int y = new_room.y; y < new_room.y + new_room.height; y++) {
                for (int x = new_room.x; x < new_room.x + new_room.width; x++) {
                    map->map[y][x] = '.';
                }
            }         
        }
        else
            create_room(&new_room, max_width, max_height);
        int intersect = 0;
        for (int j = 0; j < map->room_count; j++) {
            Room *other_room = &map->rooms[j];
            if (!(new_room.x + new_room.width < other_room->x ||
                  new_room.x > other_room->x + other_room->width ||
                  new_room.y + new_room.height < other_room->y ||
                  new_room.y > other_room->y + other_room->height)) {
                intersect = 1;
                break;
            }
        }
        if (!intersect) {
            map->rooms[map->room_count++] = new_room;
            for (int y = new_room.y; y < new_room.y + new_room.height; y++) {
                for (int x = new_room.x; x < new_room.x + new_room.width; x++) {
                    map->map[y][x] = '.';
                }
            }
        }
    }
    if(map->floor==4){
        map->rooms[MAX_ROOMS-1].is_treasure=1;
    }
    if(map->floor==1){
        map->rooms[2].has_password_door=1;
        map->rooms[2].opend=0;
        snprintf(map->rooms[2].password, 6, "%04d", rand() % 10000); // تولید رمز 4 رقمی
    }
    else{
        map->rooms[0].has_password_door=1;
        map->rooms[0].opend=0;
        snprintf(map->rooms[0].password, 6, "%04d", rand() % 10000); // تولید رمز 4 رقمی        
    }
}

void connect_rooms(Map *map) {
    for (int i = 0; i < map->room_count - 1; i++) {
        Room *room1 = &map->rooms[i];
        Room *room2 = &map->rooms[i + 1];
        int x1 = room1->x + room1->width / 2;
        int y1 = room1->y + room1->height / 2;
        int x2 = room2->x + room2->width / 2;
        int y2 = room2->y + room2->height / 2;
        
        int way_len = 0;
        while (x1 != x2) {
            if (map->map[y1][x1] == '.') {
                map->map[y1][x1] = '+';
            } else {
                map->map[y1][x1] = '#';
                way_len++;
            }

            // بررسی حرکت در راستای افقی
            if (x2 > x1 && map->map[y1][x1 + 1] != 'O') {
                x1 += 1;
            } else if (x2 < x1 && map->map[y1][x1 - 1] != 'O') {
                x1 -= 1;
            } else {
                y1 += (y2 > y1) ? 1 : -1;
            }
        }

        // حرکت در راستای عمودی
        while (y1 != y2) {
            if (map->map[y1][x1] == '.') {
                map->map[y1][x1] = '+';
            } else {
                map->map[y1][x1] = '#';
                way_len++;
            }
            y1 += (y2 > y1) ? 1 : -1;
        }
        if(way_len<16){
            int max_width, max_height;
            getmaxyx(stdscr, max_height, max_width);
            generate_map(map,max_width,max_height);
        }
    }

    // فضای داخل اتاق‌ها را پاک می‌کنیم
    for (int i = 0; i < map->room_count; i++) {
        Room *room = &map->rooms[i];
        for (int y = room->y + 1; y < room->y + room->height - 1; y++) {
            for (int x = room->x + 1; x < room->x + room->width - 1; x++) {
                map->map[y][x] = '.';
            }
        }
    }
}

void secret_room(Map *map,int i){
    Room *room = &map->rooms[i];
    for (int x = room->x; x < room->x + room->width; x++) {
        if(map->map[room->y][x] == '+'){
            map->map[room->y][x] = '?';
        }
        if( map->map[room->y + room->height - 1][x] == '+'){
            map->map[room->y + room->height - 1][x] = '?';
        }
    }
    for (int y = room->y; y < room->y + room->height; y++) {
        if(map->map[y][room->x] == '+'){
            map->map[y][room->x] = '!';
        }
        if(map->map[y][room->x + room->width - 1] == '+'){
            map->map[y][room->x + room->width - 1] = '!';
        }
    }
}
void password_room(Map *map,int i){
    Room *room = &map->rooms[i];
    int px = room->x+1 + rand() % (room->width-3);
    int py = room->y+1 + rand() % (room->height-3);
    if(px==3&&py==3){
        px=2;
        py=2;
    }
    map->map[py][px] = '&'; 
    for (int x = room->x; x < room->x + room->width; x++) {
        if(map->map[room->y][x] == '+'){
            map->map[room->y][x] = '1';
            return;
        }
        if( map->map[room->y + room->height - 1][x] == '+'){
            map->map[room->y + room->height - 1][x] = '1';
            return;
        }
    }
    for (int y = room->y; y < room->y + room->height; y++) {
        if(map->map[y][room->x] == '+'){
            map->map[y][room->x] = '1';
            return;
        }
        if(map->map[y][room->x + room->width - 1] == '+'){
            map->map[y][room->x + room->width - 1] = '1';
            return;
        }
    }
}
void add_pillars_stair_traps(Map *map) {
    for (int i = 0; i < map->room_count; i++) {
        Room *room = &map->rooms[i];
        int pillars = (room->width - 2) * (room->height - 2) / 16; // تعداد ستون‌ها
        int traps= rand()%2;
        int place_enemy=0;
        for (int j = 0; j < pillars; j++) {
            int px = room->x + 2 + rand() % (room->width - 4);
            int py = room->y + 2 + rand() % (room->height - 4);
            if (map->map[py][px] == '.') {
                map->map[py][px] = 'O'; // ستون
            }
        }
        for (int j = 0; j < traps; j++) {
            int px = room->x + 2 + rand() % (room->width - 4);
            int py = room->y + 2 + rand() % (room->height - 4);
            if (map->map[py][px] == '.') {
                map->map[py][px] = '8'; // trap
            }
        }
        if(map->rooms[i].is_enchant==1){
            for (int j = 0; j <1+rand()%3; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'h'; // :(health spell)
                }
            } 
            for (int j = 0; j < traps; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = '8'; // trap
                }
            }   
            for (int j = 0; j <1+rand()%3; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 's'; // :(speed spell)
                }
            } 
            for (int j = 0; j <1+rand()%3; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'd'; // :(damage spell)
                }
            }    
        }
        if(map->rooms[i].is_treasure==1){
            int which_enemy=rand()%5;
            if(which_enemy==0){
                for (int j = 0; j <rand()%2; j++) {
                    int px = room->x + 2 + rand() % (room->width - 4);
                    int py = room->y + 2 + rand() % (room->height - 4);
                    if (map->map[py][px] == '.') {
                        map->map[py][px] = 'D'; // (deamon)
                        map->rooms[i].enemies[0]=5;
                    }
                } 
            }
            if(which_enemy==1){
                for (int j = 0; j <rand()%2; j++) {
                    int px = room->x + 2 + rand() % (room->width - 4);
                    int py = room->y + 2 + rand() % (room->height - 4);
                    if (map->map[py][px] == '.') {
                        map->map[py][px] = 'F'; // (Monster)
                        map->rooms[i].enemies[1]=10;
                    }
                } 
            }
            if(which_enemy==2){
                for (int j = 0; j <rand()%2; j++) {
                    int px = room->x + 2 + rand() % (room->width - 4);
                    int py = room->y + 2 + rand() % (room->height - 4);
                    if (map->map[py][px] == '.') {
                        map->map[py][px] = 'g'; // (Giant)
                        map->rooms[i].enemies[2]=15;
                    }
                } 
            }
            if(which_enemy==3){
                for (int j = 0; j <rand()%2; j++) {
                    int px = room->x + 2 + rand() % (room->width - 4);
                    int py = room->y + 2 + rand() % (room->height - 4);
                    if (map->map[py][px] == '.') {
                        map->map[py][px] = 'S'; // (Snake)
                        map->rooms[i].enemies[3]=20;
                    }
                } 
            }
            if(which_enemy==4){
                for (int j = 0; j <rand()%2; j++) {
                    int px = room->x + 2 + rand() % (room->width - 4);
                    int py = room->y + 2 + rand() % (room->height - 4);
                    if (map->map[py][px] == '.') {
                        map->map[py][px] = 'U'; // (Undeed)
                        map->rooms[i].enemies[4]=30;

                    }
                } 
            }
            
            int px = room->x + 2 + rand() % (room->width - 4);
            int py = room->y + 2 + rand() % (room->height - 4);
            map->map[py][px] = 'T'; 
        }
        else{
            for (int j = 0; j < rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'F'; // FOOD
                }
            }
            for (int j = 0; j <rand()%3; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = '$'; // GOLD
                }
            }
            for (int j = 0; j <rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'g'; // GOLD
                }
            } 
            for (int j = 0; j <rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'e'; // (Dagger):
                }
            }     
            for (int j = 0; j <rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'N'; // (Magic Wand)
                }
            }    
            for (int j = 0; j <rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'A'; // (Normal Arrow)
                }
            }  
            for (int j = 0; j <rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'W'; // :(Sword)
                }
            }  
            for (int j = 0; j <rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'h'; // :(health spell)
                }
            } 
            for (int j = 0; j <rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 's'; // :(speed spell)
                }
            } 
            for (int j = 0; j <rand()%2; j++) {
                int px = room->x + 2 + rand() % (room->width - 4);
                int py = room->y + 2 + rand() % (room->height - 4);
                if (map->map[py][px] == '.') {
                    map->map[py][px] = 'd'; // :(damage spell)
                }
            } 
            if(i!=0&&map->floor>1){
                int which_enemy=rand()%5;
                if(which_enemy==0){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'D'; // (deamon)
                            map->rooms[i].enemies[0]=5;
                        }
                    } 
                }
                if(which_enemy==1){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'F'; // (Monster)
                            map->rooms[i].enemies[1]=10;
                        }
                    } 
                }
                if(which_enemy==2){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'g'; // (Giant)
                            map->rooms[i].enemies[2]=15;
                        }
                    } 
                }
                if(which_enemy==3){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'S'; // (Snake)
                            map->rooms[i].enemies[3]=20;
                        }
                    } 
                }
                if(which_enemy==4){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'U'; // (Undeed)
                            map->rooms[i].enemies[4]=30;

                        }
                    } 
                }
            }
            if(i!=2&&map->floor==1){
                int which_enemy=rand()%5;
                if(which_enemy==0){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'D'; // (deamon)
                            map->rooms[i].enemies[0]=5;
                        }
                    } 
                }
                if(which_enemy==1){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'F'; // (Monster)
                            map->rooms[i].enemies[1]=10;
                        }
                    } 
                }
                if(which_enemy==2){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'g'; // (Giant)
                            map->rooms[i].enemies[2]=15;
                        }
                    } 
                }
                if(which_enemy==3){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'S'; // (Snake)
                            map->rooms[i].enemies[3]=20;
                        }
                    } 
                }
                if(which_enemy==4){
                    for (int j = 0; j <rand()%2; j++) {
                        int px = room->x + 2 + rand() % (room->width - 4);
                        int py = room->y + 2 + rand() % (room->height - 4);
                        if (map->map[py][px] == '.') {
                            map->map[py][px] = 'U'; // (Undeed)
                            map->rooms[i].enemies[4]=30;

                        }
                    } 
                }
            }
        }
    }
    Room *room = &map->rooms[0];
    map->map[3][3] = '<'; // stair

}

void generate_map(Map *map, int max_width, int max_height) {
    for (int y = 0; y < max_height; y++) {
        for (int x = 0; x < max_width; x++) {
            map->map[y][x] = ' ';
        }
    }
    place_rooms(map, max_width, max_height);
    connect_rooms(map);
    int place_enchant=0;
    if(place_enchant==0 && map->floor==1){
        if(rand()%4<2){
            map->rooms[1].is_enchant=1;
            map->rooms[1].is_nightmare=0;
            map->rooms[1].is_regular=0;
            map->rooms[1].is_treasure=0;
            map->rooms[5].is_enchant=1;
            map->rooms[5].is_nightmare=0;
            map->rooms[5].is_regular=0;
            map->rooms[5].is_treasure=0;
        }
        else{
            map->rooms[1].is_enchant=1;
            map->rooms[1].is_nightmare=0;
            map->rooms[1].is_regular=0;
            map->rooms[1].is_treasure=0;
            map->rooms[4].is_enchant=1;
            map->rooms[4].is_nightmare=0;
            map->rooms[4].is_regular=0;
            map->rooms[4].is_treasure=0;
        }
        place_enchant=1;
    }else if(place_enchant==0 && map->floor!=1){
        if(rand()%4<2){
            map->rooms[2].is_enchant=1;
            map->rooms[2].is_nightmare=0;
            map->rooms[2].is_regular=0;
            map->rooms[2].is_treasure=0;
            map->rooms[4].is_enchant=1;
            map->rooms[4].is_nightmare=0;
            map->rooms[4].is_regular=0;
            map->rooms[4].is_treasure=0;
        }
        else{
            map->rooms[1].is_enchant=1;
            map->rooms[1].is_nightmare=0;
            map->rooms[1].is_regular=0;
            map->rooms[1].is_treasure=0;
            map->rooms[3].is_enchant=1;
            map->rooms[3].is_nightmare=0;
            map->rooms[3].is_regular=0;
            map->rooms[3].is_treasure=0;
        }
        place_enchant=1;
    }
    add_pillars_stair_traps(map);
    int have_secrest ,have_password =0;
    for (int i = 0; i < map->room_count; i++) {
        Room *room = &map->rooms[i];
        for (int y = room->y; y < room->y + room->height; y++) {
            if(map->map[y][room->x] == '#')
                map->map[y][room->x] = '+';
            if(map->map[y][room->x + room->width - 1] == '#')
                map->map[y][room->x + room->width - 1] = '+';            
            if(map->map[y][room->x] =='.')
                map->map[y][room->x] = '|';
            if(map->map[y][room->x + room->width - 1] =='.')
                map->map[y][room->x + room->width - 1] = '|';
        }
        for (int x = room->x; x < room->x + room->width; x++) {
            if(map->map[room->y][x] == '#')
                map->map[room->y][x] = '+';
            if( map->map[room->y + room->height - 1][x] == '#')
                map->map[room->y + room->height - 1][x] = '+';
            if(map->map[room->y][x] == '.')
                map->map[room->y][x] = '-';
            if( map->map[room->y + room->height - 1][x] =='.')
                map->map[room->y + room->height - 1][x] = '-';
        }
        if(room->is_enchant==1 ){
            secret_room(map,i);
            have_secrest=1;
        }
        else if(room->has_password_door==1 &&have_password==0){
            if(map->floor==1&& i==2){
                password_room(map,i);
                have_password=1;
            }
            if(map->floor>1&& i==0){
                password_room(map,i);
                have_password=1;
            }
            
        }
    }
    special_key(map);
}

void print_map(Map *map, int max_width, int max_height,char* username) {
    clear();
    if(map->floor==1){
        print_selected_room(map,username,2,visible);
        Room selected_room=map->rooms[2];
        int place_hero=0;
        for (int y = selected_room.y; y < selected_room.y + selected_room.height; y++) {
            for (int x = selected_room.x; x < selected_room.x + selected_room.width; x++) {
                if(map->map[y][x]=='.'){
                    map->map[y][x]='H';
                    if(strcmp(settings.main_color,"Red")==0){
                        attron(COLOR_PAIR(1));
                    }
                    if(strcmp(settings.main_color,"Green")==0){
                        attron(COLOR_PAIR(2));
                    }
                    if(strcmp(settings.main_color,"Blue")==0){
                        attron(COLOR_PAIR(3));
                    }
                    if(strcmp(settings.main_color,"Yellow")==0){
                        attron(COLOR_PAIR(4));
                    }
                    mvprintw(y + 1, x, "%c",map->map[y][x]);
                    if(strcmp(settings.main_color,"Red")==0){
                        attroff(COLOR_PAIR(1));
                    }
                    if(strcmp(settings.main_color,"Green")==0){
                        attroff(COLOR_PAIR(2));
                    }
                    if(strcmp(settings.main_color,"Blue")==0){
                        attroff(COLOR_PAIR(3));
                    }
                    if(strcmp(settings.main_color,"Yellow")==0){
                        attroff(COLOR_PAIR(4));
                    } 
                    place_hero=1;
                    hero.x=x;
                    hero.y=y;
                    hero.has_key=0;
                    hero.has_broken_key=0;
                    break;                
                }
            }
            if(place_hero==1)
                break;
        }
    }
    else{
        print_selected_room(map,username,0,visible);
        Room selected_room=map->rooms[0];
        int place_hero=0;
        for (int y = selected_room.y; y < selected_room.y + selected_room.height; y++) {
            for (int x = selected_room.x; x < selected_room.x + selected_room.width; x++) {
                if(map->map[y][x]=='<'){
                    map->map[y][x]='H';
                    if(strcmp(settings.main_color,"Red")==0){
                        attron(COLOR_PAIR(1));
                    }
                    if(strcmp(settings.main_color,"Green")==0){
                        attron(COLOR_PAIR(2));
                    }
                    if(strcmp(settings.main_color,"Blue")==0){
                        attron(COLOR_PAIR(3));
                    }
                    if(strcmp(settings.main_color,"Yellow")==0){
                        attron(COLOR_PAIR(4));
                    }
                    mvprintw(y + 1, x, "%c",map->map[y][x]);
                    if(strcmp(settings.main_color,"Red")==0){
                        attroff(COLOR_PAIR(1));
                    }
                    if(strcmp(settings.main_color,"Green")==0){
                        attroff(COLOR_PAIR(2));
                    }
                    if(strcmp(settings.main_color,"Blue")==0){
                        attroff(COLOR_PAIR(3));
                    }
                    if(strcmp(settings.main_color,"Yellow")==0){
                        attroff(COLOR_PAIR(4));
                    } 
                    place_hero=1;
                    hero.x=x;
                    hero.y=y;
                    hero.has_key=0;
                    hero.has_broken_key=0;
                    break;                
                }
            }
            if(place_hero==1)
                break;
        }        
    }
    refresh();
}
void print_full_map(Map *map, int max_width, int max_height,char* username) {
    clear();
    init_pair(1,COLOR_RED,COLOR_BLACK);
    init_pair(2,COLOR_GREEN,COLOR_BLACK);
    init_pair(3,COLOR_BLUE,COLOR_BLACK);
    init_pair(4,COLOR_YELLOW,COLOR_BLACK);
    for (int y = 0; y < max_height; y++) {
        for (int x = 0; x < max_width; x++) {
            if(map->map[y][x]=='9'){
                const char* key="△";
                attron(COLOR_PAIR(4));
                mvprintw(y + 1, x, "%s",key);                
                attroff(COLOR_PAIR(4));
            }else if(map->map[y][x]=='8'){
                const char* key="∧";
                attron(COLOR_PAIR(1));
                mvprintw(y + 1, x, "%s",key); 
                attroff(COLOR_PAIR(1));
            }else if(map->map[y][x]=='1'){
                const char* key="@";
                attron(COLOR_PAIR(1));
                mvprintw(y + 1, x, "%s",key);
                attroff(COLOR_PAIR(1)); 
            }else if(map->map[y][x]=='g'){
                const char* black_gold="€";
                mvprintw(y + 1, x, "%s",black_gold); 
            }else if(map->map[y][x]=='C'){
                const char* Mace="⍑";
                mvprintw(y + 1, x, "%s",Mace); 
            }else if(map->map[y][x]=='e'){
                const char* Dagger="ⴕ";
                mvprintw(y + 1, x, "%s",Dagger); 
            }else if(map->map[y][x]=='N'){
                const char* Magic="⧙";
                mvprintw(y + 1, x, "%s",Magic); 
            }else if(map->map[y][x]=='A'){
                const char* Arrow="➤";
                mvprintw(y + 1, x, "%s",Arrow); 
            }else if(map->map[y][x]=='^'){
                const char* Dagger="ⴕ";
                mvprintw(y + 1, x, "%s",Dagger); 
            }else if(map->map[y][x]=='*'){
                const char* Magic="⧙";
                mvprintw(y + 1, x, "%s",Magic); 
            }else if(map->map[y][x]=='%'){
                const char* Arrow="➤";
                mvprintw(y + 1, x, "%s",Arrow); 
            }else if(map->map[y][x]=='W'){
                const char* Sword="⟆";
                mvprintw(y + 1, x, "%s",Sword); 
            }else if(map->map[y][x]=='H'){
                if(strcmp(settings.main_color,"Red")==0){
                    attron(COLOR_PAIR(1));
                }
                if(strcmp(settings.main_color,"Green")==0){
                    attron(COLOR_PAIR(2));
                }
                if(strcmp(settings.main_color,"Blue")==0){
                    attron(COLOR_PAIR(3));
                }
                if(strcmp(settings.main_color,"Yellow")==0){
                    attron(COLOR_PAIR(4));
                }
                mvprintw(y + 1, x, "H");
                if(strcmp(settings.main_color,"Red")==0){
                    attroff(COLOR_PAIR(1));
                }
                if(strcmp(settings.main_color,"Green")==0){
                    attroff(COLOR_PAIR(2));
                }
                if(strcmp(settings.main_color,"Blue")==0){
                    attroff(COLOR_PAIR(3));
                }
                if(strcmp(settings.main_color,"Yellow")==0){
                    attroff(COLOR_PAIR(4));
                } 
            }else if(map->map[y][x]=='2'){
                attron(COLOR_PAIR(2));
                mvprintw(y + 1, x, "@");
                attroff(COLOR_PAIR(2));
            }else if(map->map[y][x]=='1'){
                attron(COLOR_PAIR(1));
                mvprintw(y + 1, x, "@");
                attroff(COLOR_PAIR(1));
            }else if(map->map[y][x]=='&'){
                attron(COLOR_PAIR(3));
                mvprintw(y + 1, x, "&");
                attroff(COLOR_PAIR(3));
            }else
                mvprintw(y + 1, x, "%c", map->map[y][x]);

        }
    }
    refresh();
}
void special_key(Map *map){
    int room_num=rand()%6;
    Room *room = &map->rooms[room_num];
    int px = room->x + 2 + rand() % (room->width - 4);
    int py = room->y + 2 + rand() % (room->height - 4);
    while(map->map[py][px] == '.'){
        px = room->x + 2 + rand() % (room->width - 4);
        py = room->y + 2 + rand() % (room->height - 4);
        if (map->map[py][px] == '.') {
            map->map[py][px] =  '9';
            break;
        }
    }
}
void print_selected_room(Map *map, char* username,int room_num,int**visible){
    Room selected_room;
    selected_room=map->rooms[room_num];
    for (int y = selected_room.y; y < selected_room.y + selected_room.height; y++) {
        for (int x = selected_room.x; x < selected_room.x + selected_room.width; x++) {
            if(map->map[y][x]=='9'){
                const char* key="△";
                attron(COLOR_PAIR(4));
                mvprintw(y + 1, x, "%s",key);                
                attroff(COLOR_PAIR(4));
                visible[y][x]=1;               
            }else if(map->map[y][x]=='8'){
                const char* tale=".";
                mvprintw(y + 1, x, "%s",tale); 
                visible[y][x]=1; 
            }else if(map->map[y][x]=='!'){
                const char* secret_door="|";
                mvprintw(y + 1, x, "%s",secret_door); 
                visible[y][x]=1; 
            }else if(map->map[y][x]=='?'){
                const char* secret_door="-";
                mvprintw(y + 1, x, "%s",secret_door); 
                visible[y][x]=1; 
            }else if(map->map[y][x]=='3'){
                const char* secret_door="?";
                mvprintw(y + 1, x, "%s",secret_door); 
                visible[y][x]=1; 
            }else if(map->map[y][x]=='t'){
                const char* visible_trap="∧";
                attron(COLOR_PAIR(1));
                mvprintw(y + 1, x, "%s",visible_trap); 
                attroff(COLOR_PAIR(1)); 
                visible[y][x]=1; 
            }else if(map->map[y][x]=='F'){
                const char* food="F";
                mvprintw(y + 1, x, "%s",food); 
                visible[y][x]=1; 
            }else if(map->map[y][x]=='g'){
                const char* black_gold="€";
                mvprintw(y + 1, x, "%s",black_gold); 
                visible[y][x]=1; 
            }else if(map->map[y][x]=='C'){
                const char* Mace="⍑";
                mvprintw(y + 1, x, "%s",Mace); 
                visible[y][x]=1; 
            }else if(map->map[y][x]=='e'){
                const char* Dagger="ⴕ";
                mvprintw(y + 1, x, "%s",Dagger); 
                visible[y][x]=1;
            }else if(map->map[y][x]=='N'){
                const char* Magic="⧙";
                mvprintw(y + 1, x, "%s",Magic); 
                visible[y][x]=1;
            }else if(map->map[y][x]=='A'){
                const char* Arrow="➤";
                mvprintw(y + 1, x, "%s",Arrow); 
                visible[y][x]=1;
            }else if(map->map[y][x]=='^'){
                const char* Dagger="ⴕ";
                mvprintw(y + 1, x, "%s",Dagger); 
                visible[y][x]=1;
            }else if(map->map[y][x]=='*'){
                const char* Magic="⧙";
                mvprintw(y + 1, x, "%s",Magic); 
                visible[y][x]=1;
            }else if(map->map[y][x]=='%'){
                const char* Arrow="➤";
                mvprintw(y + 1, x, "%s",Arrow); 
                visible[y][x]=1;
            }else if(map->map[y][x]=='W'){
                const char* Sword="⟆";
                mvprintw(y + 1, x, "%s",Sword); 
                visible[y][x]=1;
            }else if(map->map[y][x]=='2'){
                attron(COLOR_PAIR(2));
                mvprintw(y + 1, x, "@");
                attroff(COLOR_PAIR(2));
                visible[y][x]=1;
            }else if(map->map[y][x]=='1'){
                attron(COLOR_PAIR(1));
                mvprintw(y + 1, x, "@");
                attroff(COLOR_PAIR(1));
                visible[y][x]=1;
            }else if(map->map[y][x]=='&'){
                attron(COLOR_PAIR(3));
                mvprintw(y + 1, x, "&");
                attroff(COLOR_PAIR(3));
                visible[y][x]=1;
            }else if(map->map[y][x]=='H'){
                if(strcmp(settings.main_color,"Red")==0){
                    attron(COLOR_PAIR(1));
                }
                if(strcmp(settings.main_color,"Green")==0){
                    attron(COLOR_PAIR(2));
                }
                if(strcmp(settings.main_color,"Blue")==0){
                    attron(COLOR_PAIR(3));
                }
                if(strcmp(settings.main_color,"Yellow")==0){
                    attron(COLOR_PAIR(4));
                }
                mvprintw(y + 1, x, "H");
                if(strcmp(settings.main_color,"Red")==0){
                    attroff(COLOR_PAIR(1));
                }
                if(strcmp(settings.main_color,"Green")==0){
                    attroff(COLOR_PAIR(2));
                }
                if(strcmp(settings.main_color,"Blue")==0){
                    attroff(COLOR_PAIR(3));
                }
                if(strcmp(settings.main_color,"Yellow")==0){
                    attroff(COLOR_PAIR(4));
                } 
            }else
                mvprintw(y + 1, x, "%c", map->map[y][x]);
                visible[y][x]=1; 
            //map_check[y][x]=1;
        }
    }
}

int hero_movement(Map *map,int*** map_check_ptrr,int***visible_ptrr, char* username){
    /*if(strcmp(settings.main_color,"Red")==0){
        init_pair(1,COLOR_RED,COLOR_BLACK);

    }
    if(strcmp(settings.main_color,"Green")==0){
        init_pair(1,COLOR_GREEN,COLOR_BLACK);

    }
    if(strcmp(settings.main_color,"Blie")==0){
        init_pair(1,COLOR_BLUE,COLOR_BLACK);
    }
    if(strcmp(settings.main_color,"Yellow")==0){
        init_pair(1,COLOR_YELLOW,COLOR_BLACK);
    }*/
    init_pair(1,COLOR_RED,COLOR_BLACK);
    init_pair(2,COLOR_GREEN,COLOR_BLACK);
    init_pair(3,COLOR_BLUE,COLOR_BLACK);
    init_pair(4,COLOR_YELLOW,COLOR_BLACK);
    load_settings(username);
    int** map_check=*map_check_ptrr;
    int** visible=*visible_ptrr;
    strcpy(hero.color,settings.main_color);
    int ch = getch(); // کلید ورودی را بگیر
    int new_x = hero.x, new_y = hero.y;
    int last_x=hero.x, last_y = hero.y;
    int can_grab=1;
    // حرکت براساس کلید عددی
    switch (ch) {
        case '8':    
            new_y--; 
            if(speed_spel > 0){
                new_y--;
            }
            break; // حرکت به بالا (عدد 8)

        case '2':    
            new_y++; 
            if(speed_spel > 0){
                new_y++;
            }
            break; // حرکت به پایین (عدد 2)

        case '4':    
            new_x--; 
            if(speed_spel > 0){
                new_x--;
            }
            break; // حرکت به چپ (عدد 4)

        case '6':    
            new_x++; 
            if(speed_spel > 0){
                new_x++;
            }
            break; // حرکت به راست (عدد 6)

        case '7':    
            new_x--; 
            new_y--; 
            if(speed_spel > 0){
                new_x--;
                new_y--;
            }
            break; // حرکت به بالا-چپ (عدد 7)

        case '9':    
            new_x++; 
            new_y--; 
            if(speed_spel > 0){
                new_x++;
                new_y--;
            }
            break; // حرکت به بالا-راست (عدد 9)

        case '1':    
            new_x--; 
            new_y++; 
            if(speed_spel > 0){
                new_x--;
                new_y++;
            }
            break; // حرکت به پایین-چپ (عدد 1)

        case '3':    
            new_x++; 
            new_y++; 
            if(speed_spel > 0){
                new_x++;
                new_y++;
            }
            break; // حرکت به پایین-راست (عدد 3)

        case '5':    
            break; // هیچ عملی انجام نمی‌شود (عدد 5)
        case 'm':
            show_full_map = !show_full_map; // تغییر حالت نقشه
            clear(); // پاکسازی صفحه
            if (show_full_map) {
                print_full_map(map,COLS,LINES,username); // نمایش نقشه کامل
            } else {
                display_visible_map(map, visible); // نمایش نقشه قابل مشاهده
            }
            return ch;// خروج با کلید q
        case 'f':   
            timeout(-1);
            int ch_func=getch();
            init_pair(1,COLOR_RED,COLOR_BLACK);
            init_pair(2,COLOR_GREEN,COLOR_BLACK);
            init_pair(3,COLOR_BLUE,COLOR_BLACK);
            init_pair(4,COLOR_YELLOW,COLOR_BLACK);
            switch (ch_func){
                case '8': // حرکت به بالا
                    while (map_check[hero.y][hero.x] == '.' || map_check[hero.y][hero.x] == '#') {
                        hero.x = new_x;
                        hero.y = new_y;
                        map->map[hero.y][hero.x] = map_check[hero.y][hero.x]; // جای قبلی بازیکن را پاک کن
                        mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                        visible[hero.y][hero.x] = 1;
                        new_y--;
                    }
                    new_y += 2;
                    hero.x = new_x;
                    hero.y = new_y;
                    map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
                    break;

                case '2': // حرکت به پایین
                    while (map_check[hero.y][hero.x] == '.' || map_check[hero.y][hero.x] == '#') {
                        hero.x = new_x;
                        hero.y = new_y;
                        map->map[hero.y][hero.x] = map_check[hero.y][hero.x];
                        mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                        visible[hero.y][hero.x] = 1;
                        new_y++;
                    }
                    new_y -= 2;
                    hero.x = new_x;
                    hero.y = new_y;
                    map->map[hero.y][hero.x] = 'H';
                    break;

                case '4': // حرکت به چپ
                    while (map_check[hero.y][hero.x] == '.' || map_check[hero.y][hero.x] == '#') {
                        hero.x = new_x;
                        hero.y = new_y;
                        map->map[hero.y][hero.x] = map_check[hero.y][hero.x];
                        mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                        visible[hero.y][hero.x] = 1;
                        new_x--;
                    }
                    new_x += 2;
                    hero.x = new_x;
                    hero.y = new_y;
                    map->map[hero.y][hero.x] = 'H';
                    break;

                case '6': // حرکت به راست
                    while (map_check[hero.y][hero.x] == '.' || map_check[hero.y][hero.x] == '#') {
                        hero.x = new_x;
                        hero.y = new_y;
                        map->map[hero.y][hero.x] = map_check[hero.y][hero.x];
                        mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                        visible[hero.y][hero.x] = 1;
                        new_x++;
                    }
                    new_x -= 2;
                    hero.x = new_x;
                    hero.y = new_y;
                    map->map[hero.y][hero.x] = 'H';
                    break;

                case '7': // حرکت به بالا-چپ
                    while (map_check[hero.y][hero.x] == '.' || map_check[hero.y][hero.x] == '#') {
                        hero.x = new_x;
                        hero.y = new_y;
                        map->map[hero.y][hero.x] = map_check[hero.y][hero.x];
                        mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                        visible[hero.y][hero.x] = 1;
                        new_x--;
                        new_y--;
                    }
                    new_x+=2;
                    new_y+=2;
                    hero.x = new_x;
                    hero.y = new_y;
                    map->map[hero.y][hero.x] = 'H';
                    break;

                case '9': // حرکت به بالا-راست
                    while (map_check[hero.y][hero.x] == '.' || map_check[hero.y][hero.x] == '#') {
                        hero.x = new_x;
                        hero.y = new_y;
                        map->map[hero.y][hero.x] = map_check[hero.y][hero.x];
                        mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                        visible[hero.y][hero.x] = 1;
                        new_x++;
                        new_y--;
                    }
                    new_x-=2;
                    new_y+=2;
                    hero.x = new_x;
                    hero.y = new_y;
                    map->map[hero.y][hero.x] = 'H';
                    break;

                case '1': // حرکت به پایین-چپ
                    while (map_check[hero.y][hero.x] == '.' || map_check[hero.y][hero.x] == '#') {
                        hero.x = new_x;
                        hero.y = new_y;
                        map->map[hero.y][hero.x] = map_check[hero.y][hero.x];
                        mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                        visible[hero.y][hero.x] = 1;
                        new_x--;
                        new_y++;
                    }
                    new_x+2;
                    new_y-=2;
                    hero.x = new_x;
                    hero.y = new_y;
                    map->map[hero.y][hero.x] = 'H';
                    break;

                case '3': // حرکت به پایین-راست
                    while (map_check[hero.y][hero.x] == '.' || map_check[hero.y][hero.x] == '#') {
                        hero.x = new_x;
                        hero.y = new_y;
                        map->map[hero.y][hero.x] = map_check[hero.y][hero.x];
                        mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                        visible[hero.y][hero.x] = 1;
                        new_x++;
                        new_y++;
                    }
                    new_x-=2;
                    new_y-=2;
                    hero.x = new_x;
                    hero.y = new_y;
                    map->map[hero.y][hero.x] = 'H';
                    break;

                default:
                    break;
            }
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
                mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
                mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
                mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
                mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
                attroff(COLOR_PAIR(4));
            }
            return ch;

        case 'g':
            timeout(-1);
            int grab_func = getch(); // کلید ورودی را بگیر
            switch (grab_func) {
                case '8':    
                    new_y--; 
                    if(speed_spel > 0){
                        new_y--;
                    }
                    can_grab = 0; 
                    break; // حرکت به بالا (عدد 8)

                case '2':    
                    new_y++; 
                    if(speed_spel > 0){
                        new_y++;
                    }
                    can_grab = 0; 
                    break; // حرکت به پایین (عدد 2)

                case '4':    
                    new_x--; 
                    if(speed_spel > 0){
                        new_x--;
                    }
                    can_grab = 0; 
                    break; // حرکت به چپ (عدد 4)

                case '6':    
                    new_x++; 
                    if(speed_spel > 0){
                        new_x++;
                    }
                    can_grab = 0; 
                    break; // حرکت به راست (عدد 6)

                case '7':    
                    new_x--; 
                    new_y--; 
                    if(speed_spel > 0){
                        new_x--;
                        new_y--;
                    }
                    can_grab = 0; 
                    break; // حرکت به بالا-چپ (عدد 7)

                case '9':    
                    new_x++; 
                    new_y--; 
                    if(speed_spel > 0){
                        new_x++;
                        new_y--;
                    }
                    can_grab = 0; 
                    break; // حرکت به بالا-راست (عدد 9)

                case '1':    
                    new_x--; 
                    new_y++; 
                    if(speed_spel > 0){
                        new_x--;
                        new_y++;
                    }
                    can_grab = 0; 
                    break; // حرکت به پایین-چپ (عدد 1)

                case '3':    
                    new_x++; 
                    new_y++; 
                    if(speed_spel > 0){
                        new_x++;
                        new_y++;
                    }
                    can_grab = 0; 
                    break; // حرکت به پایین-راست (عدد 3)
            }
            break;
        case 's':
            timeout(-1);
            for(int i=hero.y-1;i<=hero.y+1;i++){
                for(int j=hero.x-1;j<=hero.x+1;j++){
                    if(map_check[i][j]=='8'){
                        attron(COLOR_PAIR(1));
                        const char* key="∧";
                        mvprintw(i + 1, j, "%s",key);
                        attroff(COLOR_PAIR(1));
                    }else if(map_check[i][j]=='!'){
                        attron(COLOR_PAIR(2));
                        mvprintw(i + 1, j, "?"); 
                        attroff(COLOR_PAIR(2)); 
                    }else if(map_check[i][j]=='?'){
                        attron(COLOR_PAIR(1));
                        mvprintw(i + 1, j, "?"); 
                        attroff(COLOR_PAIR(2));                   
                    }
                }
            }
            getch();
            break;
        case 'E':
            food_menu();
            display_visible_map(map,visible);
            break;
        case 'i':
            clear();
            timeout(-1);
            const char* mace="⚒";
            mvprintw(5, 3, "%s",mace); 
            const char* Sword="⚔";
            mvprintw(6,3, "%s",Sword);
            const char* Dagger="🗡";
            mvprintw(7, 3, "%s",Dagger); 
            const char* Magic_Wand="🪄";
            mvprintw(8, 3, "%s",Magic_Wand); 
            const char* Normal_Arrow="➳";
            mvprintw(9, 3, "%s",Normal_Arrow); 
            mvprintw(5,5, "(Mace)         type:short-range            dmage: 5   amount:%d    code: M",hero.weapon[0]);
            mvprintw(6,5, "(Sword)        type:short-range            dmage: 10  amount:%d    code: S",hero.weapon[4]);
            mvprintw(7,5, "(Dagger)       type:long-range  range: 5   dmage: 12  amount:%d    code: D",hero.weapon[1]);
            mvprintw(8,5, "(Magic Wand)   type:long-range  range: 10  dmage: 15  amount:%d    code: W",hero.weapon[2]);
            mvprintw(9,5, "(Normal Arrow) type:long-range  range: 5   dmage: 5   amount:%d    code: N",hero.weapon[3]);
            mvprintw(1,5, "Press i to close weapon window  w to put weapon in bag  code of weapon to choose.");
            mvprintw(1,88, "                                          ");
            if(hero.using_weapon==-1){
                mvprintw(1,88, "You don't have weapon in your hand");
            }
            if(hero.using_weapon==0){
                mvprintw(1,88, "You have MACE in your hand");
            }
            if(hero.using_weapon==1){
                mvprintw(1,88, "You have DAGGER in your hand");
            }
            if(hero.using_weapon==2){
                mvprintw(1,88, "You have MAGIC WAND in your hand");
            }
            if(hero.using_weapon==3){
                mvprintw(1,88, "You have ARROW in your hand");
            }
            if(hero.using_weapon==4){
                mvprintw(1,88, "You have SWORD in your hand");
            }
            mvprintw(2,5, "                                                            ");
            int func_w = getch();
            while(func_w!='i'){
                mvprintw(5,5, "(Mace)         type:short-range            dmage: 5   amount:%d ",hero.weapon[0]);
                mvprintw(6,5, "(Sword)        type:short-range            dmage: 10  amount:%d ",hero.weapon[4]);
                mvprintw(7,5, "(Dagger)       type:long-range  range: 5   dmage: 12  amount:%d ",hero.weapon[1]);
                mvprintw(8,5, "(Magic Wand)   type:long-range  range: 10  dmage: 15  amount:%d ",hero.weapon[2]);
                mvprintw(9,5, "(Normal Arrow) type:long-range  range: 5   dmage: 5   amount:%d ",hero.weapon[3]);
                mvprintw(1,88, "                                          ");
                if(hero.using_weapon==-1){
                    mvprintw(1,88, "You don't have weapon in your hand");
                }
                if(hero.using_weapon==0){
                    mvprintw(1,88, "You have MACE in your hand");
                }
                if(hero.using_weapon==1){
                    mvprintw(1,88, "You have DAGGER in your hand");
                }
                if(hero.using_weapon==2){
                    mvprintw(1,88, "You have MAGIC WAND in your hand");
                }
                if(hero.using_weapon==3){
                    mvprintw(1,88, "You have ARROW in your hand");
                }
                if(hero.using_weapon==4){
                    mvprintw(1,88, "You have SWORD in your hand");
                }
                mvprintw(1,5, "Press i to close weapon window  w to put weapon in bag  code of weapon to choose");
                mvprintw(2,5, "                                                            ");
                if(func_w=='w'){
                    if(hero.using_weapon!=-1){
                        hero.weapon[hero.using_weapon]+=1;
                        hero.using_weapon=-1;   
                        mvprintw(2,5, "Weapon put in bag                     ");
                    }
                    else{
                        mvprintw(2,5, "You don't have weapon to put in bag...");
                    }
                }
                if(func_w=='M'){
                    if(hero.using_weapon!=-1){
                        mvprintw(2,5, "You should put your weapon first...");
                    }
                    else{
                        if(hero.weapon[0]==0){
                            mvprintw(2,5, "You don't have Mace in your bag");
                        }
                        else{
                            hero.weapon[0]-=1;
                            hero.using_weapon=0;
                            mvprintw(2,5, "Mace is choosed                     ");  
                        }
                        
                    }
                }
                if(func_w=='S'){
                    if(hero.using_weapon!=-1){
                        mvprintw(2,5, "You should put your weapon first...");
                    }
                    else{
                        if(hero.weapon[4]==0){
                            mvprintw(2,5, "You don't have Sword in your bag");
                        }
                        else{
                            hero.weapon[4]-=1;
                            hero.using_weapon=4; 
                            mvprintw(2,5, "Sword is choosed                     ");  
                        }
                        
                    }
                }
                if(func_w=='D'){
                    if(hero.using_weapon!=-1){
                        mvprintw(2,5, "You should put your weapon first...");
                    }
                    else{
                        if(hero.weapon[1]==0){
                            mvprintw(2,5, "You don't have Dagger in your bag");
                        }
                        else{
                            hero.weapon[1]-=1;
                            hero.using_weapon=1;  
                            mvprintw(2,5, "Dagger is choosed                     ");
                        }
                        
                    }
                }
                if(func_w=='W'){
                    if(hero.using_weapon!=-1){
                        mvprintw(2,5, "You should put your weapon first...");
                    }
                    else{
                        if(hero.weapon[2]==0){
                            mvprintw(2,5, "You don't have Magic Wand in your bag");
                        }
                        else{
                            hero.weapon[2]-=1;
                            hero.using_weapon=2;
                            mvprintw(2,5, "Magic Wand is choosed                     ");  
                        }
                        
                    }
                }
                if(func_w=='N'){
                    if(hero.using_weapon!=-1){
                        mvprintw(2,5, "You should put your weapon first...");
                    }
                    else{
                        if(hero.weapon[3]==0){
                            mvprintw(2,5, "You don't have Normal Arrow in your bag");
                        }
                        else{
                            hero.weapon[3]-=1;
                            hero.using_weapon=3;  
                            mvprintw(2,5, "Normal Arrow is choosed                     ");  
                        }
                        
                    }
                }
                mvprintw(5,5, "(Mace)         type:short-range            dmage: 5   amount:%d ",hero.weapon[0]);
                mvprintw(6,5, "(Sword)        type:short-range            dmage: 10  amount:%d ",hero.weapon[4]);
                mvprintw(7,5, "(Dagger)       type:long-range  range: 5   dmage: 12  amount:%d ",hero.weapon[1]);
                mvprintw(8,5, "(Magic Wand)   type:long-range  range: 10  dmage: 15  amount:%d ",hero.weapon[2]);
                mvprintw(9,5, "(Normal Arrow) type:long-range  range: 5   dmage: 5   amount:%d ",hero.weapon[3]);
                mvprintw(1,5, "Press i to close weapon window  w to put weapon in bag  code of weapon to choose");
                if(hero.using_weapon==-1){
                    mvprintw(1,88, "You don't have weapon in your hand");
                }
                if(hero.using_weapon==0){
                    mvprintw(1,88, "You have MACE in your hand");
                }
                if(hero.using_weapon==1){
                    mvprintw(1,88, "You have DAGGER in your hand");
                }
                if(hero.using_weapon==2){
                    mvprintw(1,88, "You have MAGIC WAND in your hand");
                }
                if(hero.using_weapon==3){
                    mvprintw(1,88, "You have ARROW in your hand");
                }
                if(hero.using_weapon==4){
                    mvprintw(1,88, "You have SWORD in your hand");
                }
                func_w=getch();
            }
            display_visible_map(map,visible);
            break;
        case 'p':
            clear();
            timeout(-1);
            const char* healt_spell="💖";
            mvprintw(5, 3, "%s",healt_spell); 
            const char* speed_spell="✨";
            mvprintw(6, 3, "%s",speed_spell); 
            const char* damge_spell="☠️";
            mvprintw(7, 3, "%s",damge_spell); 

            mvprintw(5,5, "(healt_spell)   code:h   amount:%d",hero.spell[0]);
            mvprintw(6,5, "(speed_spell)   code:s   amount:%d",hero.spell[1]);
            mvprintw(7,5, "(damge_spell)   code:d   amount:%d",hero.spell[2]);
            mvprintw(1,5, "Press p to close spell window  code of spell to use");
            mvprintw(2,5, "                                                            ");
            func_w = getch();
            while(func_w!='p'){
            mvprintw(5,5, "(healt_spell)   code:h   amount:%d   ",hero.spell[0]);
            mvprintw(6,5, "(speed_spell)   code:s   amount:%d   ",hero.spell[1]);
            mvprintw(7,5, "(damge_spell)   code:d   amount:%d   ",hero.spell[2]);
                if(func_w=='h'){
                    if(hero.spell[0]==0){
                        mvprintw(2,5, "you don't have health spell             ");
                    }
                    else{
                        mvprintw(2,5, "health spell choosed     ");
                        hero.spell[0]-=1;
                        health_spel=10;
                    }
                }
                if(func_w=='s'){
                    if(hero.spell[1]==0){
                        mvprintw(2,5, "you don't have speed spell             ");
                    }
                    else{
                        mvprintw(2,5, "speed spell choosed     ");
                        hero.spell[1]-=1;
                        speed_spel=10;
                    }
                }
                if(func_w=='d'){
                    if(hero.spell[2]==0){
                        mvprintw(2,5, "you don't have damage spell             ");
                    }
                    else{
                        mvprintw(2,5, "damage spell choosed    ");
                        hero.spell[2]-=1;
                        damage_spel=10;
                    }
                }
                mvprintw(5,5, "(healt_spell)   code:h   amount:%d   ",hero.spell[0]);
                mvprintw(6,5, "(speed_spell)   code:s   amount:%d   ",hero.spell[1]);
                mvprintw(7,5, "(damge_spell)   code:d   amount:%d   ",hero.spell[2]);
                func_w=getch();
            }
            display_visible_map(map,visible);
            break;
        
        case ' ':
            if(hero.using_weapon==-1){
                mvprintw(0,0, "Don't have weapon                     ");
            }
            else{
                if(hero.using_weapon==0||hero.using_weapon==4){
                    for(int i=hero.y-1 ; i<=hero.y+1 ; i++){
                        for(int j = hero.x-1 ; j <= hero.x+1;j++){
                            if(map->map[i][j]=='D'){
                                if(hero.using_weapon==0){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=5;
                                }
                                if(hero.using_weapon==4){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=10;
                                }
                                if(damage_spel!=0){
                                    if(hero.using_weapon==0){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=5;
                                    }
                                    if(hero.using_weapon==4){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=10;
                                    }                                    
                                }
                                if(map->rooms[which_room(map,hero.x,hero.y)].enemies[0]<=0){
                                    map->map[i][j]='.';
                                    map_check[i][j]='.';
                                }
                            }
                            if(map->map[i][j]=='E'){
                                if(hero.using_weapon==0){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[1]-=5;
                                }
                                if(hero.using_weapon==4){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[1]-=10;
                                }
                                if(damage_spel!=0){
                                    if(hero.using_weapon==0){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=5;
                                    }
                                    if(hero.using_weapon==4){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=10;
                                    }                                    
                                }
                                if(map->rooms[which_room(map,hero.x,hero.y)].enemies[1]<=0){
                                    map->map[i][j]='.';
                                    map_check[i][j]='.';
                                }
                            }
                            if(map->map[i][j]=='G'){
                                if(hero.using_weapon==0){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[2]-=5;
                                }
                                if(hero.using_weapon==4){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[2]-=10;
                                }
                                if(damage_spel!=0){
                                    if(hero.using_weapon==0){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=5;
                                    }
                                    if(hero.using_weapon==4){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=10;
                                    }                                    
                                }
                                if(map->rooms[which_room(map,hero.x,hero.y)].enemies[2]<=0){
                                    map->map[i][j]='.';
                                    map_check[i][j]='.';
                                }
                            }
                            if(map->map[i][j]=='S'){
                                if(hero.using_weapon==0){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[3]-=5;
                                }
                                if(hero.using_weapon==4){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[3]-=10;
                                }
                                if(damage_spel!=0){
                                    if(hero.using_weapon==0){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=5;
                                    }
                                    if(hero.using_weapon==4){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=10;
                                    }                                    
                                }
                                if(map->rooms[which_room(map,hero.x,hero.y)].enemies[3]<=0){
                                    map->map[i][j]='.';
                                    map_check[i][j]='.';
                                }
                            }
                            if(map->map[i][j]=='U'){
                                if(hero.using_weapon==0){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[4]-=5;
                                }
                                if(hero.using_weapon==4){
                                    map->rooms[which_room(map,hero.x,hero.y)].enemies[4]-=10;
                                }
                                if(damage_spel!=0){
                                    if(hero.using_weapon==0){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=5;
                                    }
                                    if(hero.using_weapon==4){
                                        map->rooms[which_room(map,hero.x,hero.y)].enemies[0]-=10;
                                    }                                    
                                }
                                if(map->rooms[which_room(map,hero.x,hero.y)].enemies[4]<=0){
                                    map->map[i][j]='.';
                                    map_check[i][j]='.';
                                }
                            }
                        }
                    }
                }else{
                    int range=0;
                    if(hero.using_weapon=='1'||hero.using_weapon=='3'){
                        range=5;
                    }
                    if(hero.using_weapon=='2'){
                        range=10;
                    }//(map->map[i-1][j]=='.'||map->map[i-1][j]=='D'||map->map[i-1][j]=='E'||map->map[i-1][j]=='G'||map->map[i-1][j]=='S'||map->map[i-1][j]=='U')&&range>0
                    timeout(-1);
                    diraction=getch();
                    int i=hero.x;
                    int j=hero.y;
                    int dameged=0;
                    switch(diraction){
                        case '8':
                            while(map->map[i-1][j]=='.'&&range>0){
                                i--;
                                range--;
                                if (i < 0 || i >= LINES || j < 0 || j >= COLS) {
                                    break;
                                }
                                if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                    dameged=1;
                                    break;
                                }
                            }      
                            break;
                        case '9':
                            while((map->map[i-1][j+1]=='.'||map->map[i-1][j+1]=='D'||map->map[i-1][j+1]=='E'||map->map[i-1][j+1]=='G'||map->map[i-1][j+1]=='S'||map->map[i-1][j+1]=='U')&&range>0){
                                i--;
                                j++;
                                range--;
                                if (i < 0 || i >= LINES || j < 0 || j >= COLS) {
                                    break;
                                }
                                if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                    dameged=1;
                                    break;
                                }
                            }
                            break;
                        case '6':
                            while((map->map[i][j+1]=='.'||map->map[i][j+1]=='D'||map->map[i][j+1]=='E'||map->map[i][j+1]=='G'||map->map[i][j+1]=='S'||map->map[i][j+1]=='U')&&range>0){
                                j++;
                                range--;
                                if (i < 0 || i >= LINES || j < 0 || j >= COLS) {
                                    break;
                                }
                                if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                    dameged=1;
                                    break;
                                }
                            }
                            break;
                        case '3':
                            while((map->map[i+1][j+1]=='.'||map->map[i+1][j+1]=='D'||map->map[i+1][j+1]=='E'||map->map[i+1][j+1]=='G'||map->map[i+1][j+1]=='S'||map->map[i+1][j+1]=='U')&&range>0){
                                i++;
                                j++;
                                range--;
                                if (i < 0 || i >= LINES || j < 0 || j >= COLS) {
                                    break;
                                }
                                if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                    dameged=1;
                                    break;
                                }
                            }
                            break;
                        case '2':
                            while((map->map[i+1][j]=='.'||map->map[i+1][j]=='D'||map->map[i+1][j]=='E'||map->map[i+1][j]=='G'||map->map[i+1][j]=='S'||map->map[i+1][j]=='U')&&range>0){
                                i++;
                                range--;
                                if (i < 0 || i >= LINES || j < 0 || j >= COLS) {
                                    break;
                                }
                                if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                    dameged=1;
                                    break;
                                }
                            }
                            break;
                        case '1':
                            while((map->map[i+1][j-1]=='.'||map->map[i+1][j-1]=='D'||map->map[i+1][j-1]=='E'||map->map[i+1][j-1]=='G'||map->map[i+1][j-1]=='S'||map->map[i+1][j-1]=='U')&&range>0){
                                i++;
                                j--;
                                range--;
                                if (i < 0 || i >= LINES || j < 0 || j >= COLS) {
                                    break;
                                }
                                if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                    dameged=1;
                                    break;
                                }
                            }
                            break;
                        case '4':
                            while((map->map[i][j-1]=='.'||map->map[i][j-1]=='D'||map->map[i][j-1]=='E'||map->map[i][j-1]=='G'||map->map[i][j-1]=='S'||map->map[i][j-1]=='U')&&range>0){
                                j--;
                                range--;
                                if (i < 0 || i >= LINES || j < 0 || j >= COLS) {
                                    break;
                                }
                                if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                    dameged=1;
                                    break;
                                }
                            }
                            break;
                        case '7':
                            while((map->map[i-1][j-1]=='.'||map->map[i-1][j-1]=='D'||map->map[i-1][j-1]=='E'||map->map[i-1][j-1]=='G'||map->map[i-1][j-1]=='S'||map->map[i-1][j-1]=='U')&&range>0){
                                i--;
                                j--;
                                range--;
                                if (i < 0 || i >= LINES || j < 0 || j >= COLS) {
                                    break;
                                }
                                if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                    dameged=1;
                                    break;
                                }
                            }
                            break;

                        default:
                            break;
                    }
                    if(map->map[i][j]=='D'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[0]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[0]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[0]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[0]-=5;
                        }
                        if(damage_spel!=0){
                            if(hero.using_weapon==1){
                                map->rooms[which_room(map,j,i)].enemies[0]-=12;
                            }
                            if(hero.using_weapon==2){
                                map->rooms[which_room(map,j,i)].enemies[0]-=15;
                                map->rooms[which_room(map,j,i)].enemies_move[0]=-1;
                            }
                            if(hero.using_weapon==3){
                                map->rooms[which_room(map,j,i)].enemies[0]-=5;
                            }
                        }
                    }
                    if(map->map[i][j]=='E'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[1]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[1]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[1]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[1]-=5;
                        }
                        if(damage_spel!=0){
                            if(hero.using_weapon==1){
                                map->rooms[which_room(map,j,i)].enemies[0]-=12;
                            }
                            if(hero.using_weapon==2){
                                map->rooms[which_room(map,j,i)].enemies[0]-=15;
                                map->rooms[which_room(map,j,i)].enemies_move[0]=-1;
                            }
                            if(hero.using_weapon==3){
                                map->rooms[which_room(map,j,i)].enemies[0]-=5;
                            }
                        }
                    }
                    if(map->map[i][j]=='G'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[2]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[2]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[2]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[2]-=5;
                        }
                        if(damage_spel!=0){
                            if(hero.using_weapon==1){
                                map->rooms[which_room(map,j,i)].enemies[0]-=12;
                            }
                            if(hero.using_weapon==2){
                                map->rooms[which_room(map,j,i)].enemies[0]-=15;
                                map->rooms[which_room(map,j,i)].enemies_move[0]=-1;
                            }
                            if(hero.using_weapon==3){
                                map->rooms[which_room(map,j,i)].enemies[0]-=5;
                            }
                        }
                    }
                    if(map->map[i][j]=='S'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[3]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[3]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[3]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[3]-=5;
                        }
                        if(damage_spel!=0){
                            if(hero.using_weapon==1){
                                map->rooms[which_room(map,j,i)].enemies[0]-=12;
                            }
                            if(hero.using_weapon==2){
                                map->rooms[which_room(map,j,i)].enemies[0]-=15;
                                map->rooms[which_room(map,j,i)].enemies_move[0]=-1;
                            }
                            if(hero.using_weapon==3){
                                map->rooms[which_room(map,j,i)].enemies[0]-=5;
                            }
                        }
                    }
                    if(map->map[i][j]=='U'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[4]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[4]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[4]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[4]-=5;
                        }
                        if(damage_spel!=0){
                            if(hero.using_weapon==1){
                                map->rooms[which_room(map,j,i)].enemies[0]-=12;
                            }
                            if(hero.using_weapon==2){
                                map->rooms[which_room(map,j,i)].enemies[0]-=15;
                                map->rooms[which_room(map,j,i)].enemies_move[0]=-1;
                            }
                            if(hero.using_weapon==3){
                                map->rooms[which_room(map,j,i)].enemies[0]-=5;
                            }
                        }
                    }
                    if(dameged==0){
                        if(hero.using_weapon==1){
                            map->map[i][j]='^';
                            map_check[i][j]='^';
                        }
                        if(hero.using_weapon==2){
                            map->map[i][j]='*';
                            map_check[i][j]='*';
                        }
                        if(hero.using_weapon==3){
                            map->map[i][j]='%';
                            map_check[i][j]='%';
                        }
                    }
                    //mvprintw(i+1,j,"%c", map->map[i][j]);
                }
            }
            break;
        case 'a':
            timeout(-1);
            if(hero.using_weapon==-1){
                mvprintw(0,0, "Don't have weapon                     ");
            }
            else{
                if(hero.using_weapon==0||hero.using_weapon==4){
                    mvprintw(0,0, "choose long-range weapon                     ");
                }
                else{
                    int range;
                    if(hero.using_weapon=='1'||hero.using_weapon=='3'){
                        range=5;
                    }
                    if(hero.using_weapon=='2'){
                        range=10;
                    }
                    timeout(-1);
                    int i=hero.x;
                    int j=hero.y;
                    int dameged=0;
                    if(diraction=='8'){
                        while((map->map[i-1][j]=='.'||map->map[i-1][j]=='D'||map->map[i-1][j]=='E'||map->map[i-1][j]=='G'||map->map[i-1][j]=='S'||map->map[i-1][j]=='U')&&range>0){
                            i--;
                            range--;
                            if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                dameged=1;
                                break;
                            }
                        }      
                    }
                    if(diraction=='9'){
                        while((map->map[i-1][j+1]=='.'||map->map[i-1][j+1]=='D'||map->map[i-1][j+1]=='E'||map->map[i-1][j+1]=='G'||map->map[i-1][j+1]=='S'||map->map[i-1][j+1]=='U')&&range>0){
                            i--;
                            j++;
                            range--;
                            if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                dameged=1;
                                break;
                            }
                        }
                    }
                    if(diraction=='6'){
                        while((map->map[i][j+1]=='.'||map->map[i][j+1]=='D'||map->map[i][j+1]=='E'||map->map[i][j+1]=='G'||map->map[i][j+1]=='S'||map->map[i][j+1]=='U')&&range>0){
                            j++;
                            range--;
                            if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                dameged=1;
                                break;
                            }
                        }
                    }
                    if(diraction=='3'){
                        while((map->map[i+1][j+1]=='.'||map->map[i+1][j+1]=='D'||map->map[i+1][j+1]=='E'||map->map[i+1][j+1]=='G'||map->map[i+1][j+1]=='S'||map->map[i+1][j+1]=='U')&&range>0){
                            i++;
                            j++;
                            range--;
                            if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                dameged=1;
                                break;
                            }
                        }
                    }
                    if(diraction=='2'){
                        while((map->map[i+1][j]=='.'||map->map[i+1][j]=='D'||map->map[i+1][j]=='E'||map->map[i+1][j]=='G'||map->map[i+1][j]=='S'||map->map[i+1][j]=='U')&&range>0){
                            i++;
                            range--;
                            if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                dameged=1;
                                break;
                            }
                        }
                    }
                    if(diraction=='1'){
                        while((map->map[i+1][j-1]=='.'||map->map[i+1][j-1]=='D'||map->map[i+1][j-1]=='E'||map->map[i+1][j-1]=='G'||map->map[i+1][j-1]=='S'||map->map[i+1][j-1]=='U')&&range>0){
                            i++;
                            j--;
                            range--;
                            if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                dameged=1;
                                break;
                            }
                        }
                    }
                    if(diraction=='4'){
                        while((map->map[i][j-1]=='.'||map->map[i][j-1]=='D'||map->map[i][j-1]=='E'||map->map[i][j-1]=='G'||map->map[i][j-1]=='S'||map->map[i][j-1]=='U')&&range>0){
                            j--;
                            range--;
                            if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                dameged=1;
                                break;
                            }
                        }
                    }
                    if(diraction=='7'){
                        while((map->map[i-1][j-1]=='.'||map->map[i-1][j-1]=='D'||map->map[i-1][j-1]=='E'||map->map[i-1][j-1]=='G'||map->map[i-1][j-1]=='S'||map->map[i-1][j-1]=='U')&&range>0){
                            i--;
                            j--;
                            range--;
                            if(map->map[i][j]=='D'||map->map[i][j]=='E'||map->map[i][j]=='G'||map->map[i][j]=='S'||map->map[i][j]=='U'){
                                dameged=1;
                                break;
                            }
                        }
                    }
                    if(map->map[i][j]=='D'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[0]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[0]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[0]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[0]-=5;
                        }
                    }
                    if(map->map[i][j]=='E'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[1]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[1]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[1]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[1]-=5;
                        }
                    }
                    if(map->map[i][j]=='G'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[2]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[2]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[2]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[2]-=5;
                        }
                    }
                    if(map->map[i][j]=='S'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[3]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[3]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[3]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[3]-=5;
                        }
                    }
                    if(map->map[i][j]=='U'){
                        if(hero.using_weapon==1){
                            map->rooms[which_room(map,j,i)].enemies[4]-=12;
                        }
                        if(hero.using_weapon==2){
                            map->rooms[which_room(map,j,i)].enemies[4]-=15;
                            map->rooms[which_room(map,j,i)].enemies_move[4]=-1;
                        }
                        if(hero.using_weapon==3){
                            map->rooms[which_room(map,j,i)].enemies[4]-=5;
                        }
                    }
                    if(dameged==0){
                        if(hero.using_weapon==1){
                            map->map[i][j]='^';
                            map_check[i][j]='^';
                        }
                        if(hero.using_weapon==2){
                            map->map[i][j]='*';
                            map_check[i][j]='*';
                        }
                        if(hero.using_weapon==3){
                            map->map[i][j]='%';
                            map_check[i][j]='%';
                        }
                    }
                }
            }
        break;
        
    }
    // بررسی حرکت معتبر
    if (is_valid_move(map, new_y, new_x)) {    
        map->map[hero.y][hero.x] = map_check[hero.y][hero.x]; // جای قبلی بازیکن را پاک کن
        if(map->map[hero.y][hero.x]=='2'){
            attron(COLOR_PAIR(2));
            mvprintw(hero.y + 1, hero.x, "@");
            attroff(COLOR_PAIR(2));
        }
        else if(map->map[hero.y][hero.x]=='3'){
            mvprintw(hero.y + 1, hero.x, "?");
        }
        else
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
        hero.x = new_x;
        hero.y = new_y;
        if(ch=='1'||ch=='2'||ch=='3'||ch=='4'||ch=='5'||ch=='6'||ch=='7'||ch=='8'||ch=='9'){
            int damged=0;
            for(int i=hero.y-1;i<=hero.y+1;i++){
                for (int j= hero.x-1; j <= hero.x+1; j++)
                {
                    if(map->map[i][j]=='D'){
                        hero.health-=1;
                        damged=1;
                        mvprintw(0,0, "Deamon hurts you!");
                        break;
                    }
                    if(map->map[i][j]=='E'){
                        hero.health-=2;
                        damged=1;
                        mvprintw(0,0, "Fire breath hurts you!");
                        break;
                    }
                    if(map->map[i][j]=='G'){
                        hero.health-=3;
                        damged=1;
                        mvprintw(0,0, "Gaint hurts you!");
                        break;
                    }
                    if(map->map[i][j]=='S'){
                        hero.health-=3;
                        damged=1;
                        mvprintw(0,0, "Snake hurts you!");
                        break;
                    }
                    if(map->map[i][j]=='U'){
                        hero.health-=4;
                        damged=1;
                        mvprintw(0,0, "Undeed hurts you!");
                        break;
                    }
                }
                if(damged==1){
                    break;
                }
            }
        }
        if(map->map[hero.y][hero.x]=='.'){
            int room_num=which_room(map,hero.x,hero.y);
            if(map->rooms[room_num].is_enchant==1){
                mvprintw(0,0, "Enchant room found   ");
                in_enchant_room=1;
            }
            if(map->rooms[room_num].is_treasure==1){
                mvprintw(0,0, "treasure room found   ");
                in_enchant_room=0;
            }
            else{
                mvprintw(0,0, "Regular room found   ");
                in_enchant_room=0;
            }
            int close_turn=5;
            print_selected_room(map,username,room_num,visible);
            move_r=0;
            move_d=0;
            move_u=0;
            move_l=0;
            if(ch=='1'||ch=='2'||ch=='3'||ch=='4'||ch=='5'||ch=='6'||ch=='7'||ch=='8'||ch=='9'){
                for (int i = map->rooms[room_num].y; i < map->rooms[room_num].y+map->rooms[room_num].height; i++)
                {
                    for (int j = map->rooms[room_num].x; j < map->rooms[room_num].x+map->rooms[room_num].width; j++)
                    {
                        if(map->map[i][j]=='D'&&chase_steps_deamon>1){
                            int dx = hero.x - j;
                            int dy = hero.y - i;
                            if (dx > 1) {
                                if(map->map[i][j+1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j+1]='D';
                                    map_check[i][j]='.';
                                    map_check[i][j+1]='D';
                                    chase_steps_deamon--;
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='D';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='D';
                                        chase_steps_deamon--;
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='D';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='D';
                                        chase_steps_deamon--;
                                    }
                                }
                            }else if (dx < -1){
                                if(map->map[i][j-1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j-1]='D';
                                    map_check[i][j]='.';
                                    map_check[i][j-1]='D';
                                    chase_steps_deamon--;
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='D';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='D';
                                        chase_steps_deamon--;
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='D';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='D';
                                        chase_steps_deamon--;
                                    }  
                                }
                            }
                            else if (dy > 1){
                                if(map->map[i+1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i+1][j]='D';
                                    map_check[i][j]='.';
                                    map_check[i+1][j]='D';
                                    chase_steps_deamon--;
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='D';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='D';
                                        chase_steps_deamon--;
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='D';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='D';
                                        chase_steps_deamon--;
                                    }  
                                }
                            }else if (dy < -1){
                                if(map->map[i-1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i-1][j]='D';
                                    map_check[i][j]='.';
                                    map_check[i-1][j]='D';
                                    chase_steps_deamon--;
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='D';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='D';
                                        chase_steps_deamon--;
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='D';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='D';
                                        chase_steps_deamon--;
                                    }  
                                }
                            } 
                        }
                        if(map->map[i][j]=='E'&&chase_steps_deamon>1){
                            int dx = hero.x - j;
                            int dy = hero.y - i;
                            if (dx > 1) {
                                if(map->map[i][j+1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j+1]='E';
                                    map_check[i][j]='.';
                                    map_check[i][j+1]='E';
                                    chase_steps_fire--;
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='E';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='E';
                                        chase_steps_fire--;
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='E';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='E';
                                        chase_steps_fire--;
                                    }
                                }
                            }else if (dx < -1){
                                if(map->map[i][j-1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j-1]='E';
                                    map_check[i][j]='.';
                                    map_check[i][j-1]='E';
                                    chase_steps_fire--;
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='E';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='E';
                                        chase_steps_fire--;
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='E';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='E';
                                        chase_steps_fire--;
                                    }  
                                }
                            }
                            else if (dy > 1){
                                if(map->map[i+1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i+1][j]='E';
                                    map_check[i][j]='.';
                                    map_check[i+1][j]='E';
                                    chase_steps_fire--;
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='E';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='E';
                                        chase_steps_fire--;
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='E';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='E';
                                        chase_steps_fire--;
                                    }  
                                }
                            }else if (dy < -1){
                                if(map->map[i-1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i-1][j]='E';
                                    map_check[i][j]='.';
                                    map_check[i-1][j]='E';
                                    chase_steps_fire--;
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='E';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='E';
                                        chase_steps_fire--;
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='E';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='E';
                                        chase_steps_fire--;
                                    }  
                                }
                            } 
                        }
                        if(map->map[i][j]=='G'&&chase_steps_giant>1){
                            int dx = hero.x - j;
                            int dy = hero.y - i;
                            if (dx > 1) {
                                if(map->map[i][j+1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j+1]='G';
                                    map_check[i][j]='.';
                                    map_check[i][j+1]='G';
                                    chase_steps_giant--;
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='G';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='G';
                                        chase_steps_giant--;
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='G';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='G';
                                        chase_steps_giant--;
                                    }
                                }
                            }else if (dx < -1){
                                if(map->map[i][j-1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j-1]='G';
                                    map_check[i][j]='.';
                                    map_check[i][j-1]='G';
                                    chase_steps_giant--;
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='G';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='G';
                                        chase_steps_giant--;
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='G';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='G';
                                        chase_steps_giant--;
                                    }  
                                }
                            }
                            else if (dy > 1){
                                if(map->map[i+1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i+1][j]='G';
                                    map_check[i][j]='.';
                                    map_check[i+1][j]='G';
                                    chase_steps_giant--;
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='G';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='G';
                                        chase_steps_giant--;
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='G';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='G';
                                        chase_steps_giant--;
                                    }  
                                }
                            }else if (dy < -1){
                                if(map->map[i-1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i-1][j]='G';
                                    map_check[i][j]='.';
                                    map_check[i-1][j]='G';
                                    chase_steps_giant--;
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='G';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='G';
                                        chase_steps_giant--;
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='G';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='G';
                                        chase_steps_giant--;
                                    }  
                                }
                            } 
                        }
                        if(map->map[i][j]=='S'){
                            int dx = hero.x - j;
                            int dy = hero.y - i;
                            if (dx > 1) {
                                if(map->map[i][j+1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j+1]='S';
                                    map_check[i][j]='.';
                                    map_check[i][j+1]='S';
                                    
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='S';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='S';
                                        
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='S';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='S';
                                        
                                    }
                                }
                            }else if (dx < -1){
                                if(map->map[i][j-1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j-1]='S';
                                    map_check[i][j]='.';
                                    map_check[i][j-1]='S';
                                    
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='S';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='S';
                                        
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='S';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='S';
                                        
                                    }  
                                }
                            }
                            else if (dy > 1){
                                if(map->map[i+1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i+1][j]='S';
                                    map_check[i][j]='.';
                                    map_check[i+1][j]='S';
                                    
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='S';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='S';
                                        
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='S';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='S';
                                        
                                    }  
                                }
                            }else if (dy < -1){
                                if(map->map[i-1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i-1][j]='S';
                                    map_check[i][j]='.';
                                    map_check[i-1][j]='S';
                                    
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='S';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='S';
                                        
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='S';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='S';
                                        
                                    }  
                                }
                            } 
                        } 
                        if(map->map[i][j]=='U'&&chase_steps_undeed>1){
                            int dx = hero.x - j;
                            int dy = hero.y - i;
                            if (dx > 1) {
                                if(map->map[i][j+1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j+1]='U';
                                    map_check[i][j]='.';
                                    map_check[i][j+1]='U';
                                    chase_steps_undeed--;
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='U';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='U';
                                        chase_steps_undeed--;
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='U';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='U';
                                        chase_steps_undeed--;
                                    }
                                }
                            }else if (dx < -1){
                                if(map->map[i][j-1]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i][j-1]='U';
                                    map_check[i][j]='.';
                                    map_check[i][j-1]='U';
                                    chase_steps_undeed--;
                                }else if(dy >= 1){
                                    if(map->map[i+1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i+1][j]='U';
                                        map_check[i][j]='.';
                                        map_check[i+1][j]='U';
                                        chase_steps_undeed--;
                                    }
                                }else if(dy <= -1){
                                    if(map->map[i-1][j]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i-1][j]='U';
                                        map_check[i][j]='.';
                                        map_check[i-1][j]='U';
                                        chase_steps_undeed--;
                                    }  
                                }
                            }
                            else if (dy > 1){
                                if(map->map[i+1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i+1][j]='U';
                                    map_check[i][j]='.';
                                    map_check[i+1][j]='U';
                                    chase_steps_undeed--;
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='U';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='U';
                                        chase_steps_undeed--;
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='U';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='U';
                                        chase_steps_undeed--;
                                    }  
                                }
                            }else if (dy < -1){
                                if(map->map[i-1][j]=='.'){
                                    map->map[i][j]='.';
                                    map->map[i-1][j]='U';
                                    map_check[i][j]='.';
                                    map_check[i-1][j]='U';
                                    chase_steps_undeed--;
                                }else if(dx >= 1){
                                    if(map->map[i][j+1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j+1]='U';
                                        map_check[i][j]='.';
                                        map_check[i][j+1]='U';
                                        chase_steps_undeed--;
                                    }
                                }else if(dx <= -1){
                                    if(map->map[i][j-1]=='.'){
                                        map->map[i][j]='.';
                                        map->map[i][j-1]='U';
                                        map_check[i][j]='.';
                                        map_check[i][j-1]='U';
                                        chase_steps_undeed--;
                                    }  
                                }
                            } 
                        }
                    }                
                }
            }
        }
        else if(map_check[hero.y][hero.x]=='#'){
            in_enchant_room=0;
            chase_steps_deamon=5;
            chase_steps_giant=5;
            chase_steps_fire=5; 
            chase_steps_undeed=5;
            int x =hero.x;
            int y=hero.y;
            int i=0;
            while(i<5){
                if(map_check[y][x+1]=='#'&&move_l==0){
                    while(i<5){
                        mvprintw(0,0, "goin right %d",i);
                        if(map_check[y][x+1]!='#'){
                            break;
                        }
                        else{
                            x++;
                            mvprintw(y + 1, x, "%c", map->map[y][x]);
                            visible[y][x]=1; 
                            i++;
                        }
                    }
                    move_r=1;
                    move_d=0;
                    move_u=0;
                    move_l=0;
                }
                if(map_check[y+1][x]=='#'&& move_u==0){
                   while(i<5){
                        mvprintw(0,0, "goin down  %d",i);
                        if(map_check[y+1][x]=='#'){
                            y++;
                            mvprintw(y + 1, x, "%c", map->map[y][x]);
                            visible[y][x]=1; 
                            i++;
                            continue;
                        }
                        break;
                    }
                    move_r=0;
                    move_d=1;
                    move_u=0;
                    move_l=0;
                }
                if(map_check[y-1][x]=='#'&&move_d==0){
                   while(i<5){
                        mvprintw(0,0, "goin upward%d",i);
                        if(map_check[y-1][x]!='#'){
                            break;
                        }
                        else{
                            y-=1;
                            mvprintw(y + 1, x, "%c", map->map[y][x]);
                            visible[y][x]=1; 
                            i++;
                        }
                    }
                    move_r=0;
                    move_d=0;
                    move_u=1;
                    move_l=0;
                }
                if(map_check[y][x-1]=='#'&&move_r==0){
                  while(i<5){
                        mvprintw(0,0, "goin left  %d",i);
                        if(map_check[y][x-1]!='#'){
                            break;
                        }
                        else{
                            x-=1;
                            mvprintw(y + 1, x, "%c", map->map[y][x]);
                            visible[y][x]=1; 
                            i++;
                        }
                    }
                    move_r=0;
                    move_d=0;
                    move_u=0;
                    move_l=1;
                }
                else{
                    break;
                    mvprintw(0,0, "               ");
                }
                i++;
            }
            
        }

        else if(map_check[hero.y][hero.x]=='&'){
            timeout(10);
            Room* room = &map->rooms[which_room(map,hero.x,hero.y)];
            pthread_mutex_lock(&mutex);
            if (!code_shown) {
                snprintf(room->password, 6, "%04d", rand() % 10000); // تولید رمز 4 رقمی
                char reverse_password[4];
                strcpy(reverse_password,room->password);
                if(rand()%6==0){
                    for(int i=0;i<4;i++){
                        reverse_password[i]=room->password[3-i];
                    }
                }
                mvprintw(0,COLS-10, "Code: %s", reverse_password); // نمایش رمز
                code_shown = true; // ثبت رمز به عنوان نمایش داده شده
                code_start_time = time(NULL); // زمان شروع نمایش رمز
            }
            pthread_mutex_unlock(&mutex);
            refresh();
        }
        else if(map_check[hero.y][hero.x]=='1'){
            mvprintw(last_y + 1, last_x,"H");
            mvprintw(0,COLS-10, "          ");
            mvprintw(0,0, "                                                                     ");
            mvprintw(0,0, "Enter code:");
            char input[10]; // ذخیره ورودی کاربر
            int attempts = 1;
            cbreak();
            keypad(stdscr, TRUE);
            Room* room = &map->rooms[which_room(map,hero.x,hero.y)];
            while (room->opend==0 && attempts <= MAX_ATTEMPTS) {
                if(hero.has_key==1){
                    if(rand()%10==0){
                        hero.has_key-=1;
                        hero.has_broken_key+=1;
                        mvprintw(0,0,"KEY BREAK! You have %d KEY.",hero.has_key);
                        napms(2000); 
                    }
                    else{
                        init_pair(4,COLOR_GREEN,COLOR_BLACK);
                        attron(COLOR_PAIR(4));
                        mvprintw(0,0,"Door unlocked by key! You may proceed.");
                        refresh();
                        attroff(COLOR_PAIR(4));
                        napms(2000); 
                        room->opend=1;
                        hero.has_key-=1;
                        mvprintw(0,0,"                                              ");
                        break;
                    }
                }
                mvprintw(0,0, "                                                                                                       ");
                mvprintw(0,0, "Enter code:");
                echo();
                curs_set(1);
                timeout(-1);
                mvgetnstr(0, 12, input, 4); // ورودی کاربر با طول محدود
                noecho();
                curs_set(0);
                if (strcmp(input, room->password) == 0) {
                    init_pair(4,COLOR_GREEN,COLOR_BLACK);
                    attron(COLOR_PAIR(4));
                    mvprintw(0,17,"Door unlocked by code! You may proceed.");
                    refresh();
                    attroff(COLOR_PAIR(4));
                    napms(2000); 
                    room->opend=1;
                    break;
                } else {
                        if(attempts==1){
                            init_pair(1,COLOR_YELLOW,COLOR_BLACK);
                            attron(COLOR_PAIR(1));
                            mvprintw(0,17,"EROR(Two left)");
                            refresh();
                            attroff(COLOR_PAIR(1));
                            napms(2000);
                        }
                        else if(attempts==2){
                            init_pair(2,COLOR_RED,COLOR_YELLOW);
                            attron(COLOR_PAIR(2));
                            mvprintw(0,17,"EROR(one left)");
                            refresh();
                            attroff(COLOR_PAIR(2));
                            napms(2000);
                        }
                        else if(attempts==3){
                            init_pair(3,COLOR_RED,COLOR_BLACK);
                            attron(COLOR_PAIR(3));
                            mvprintw(0,17,"EROR(SECURITY MODE)");
                            refresh();
                            attroff(COLOR_PAIR(3));
                            napms(2000);    
                        }
                        attempts++;
                }
                mvprintw(0,0, "                                                                                                       ");

            }
            if(room->opend==1){
                mvprintw(last_y + 1, last_x, "%c",map->map[last_y][last_x]);
                map_check[hero.y][hero.x]='2';
            }
            else{
                hero.x = last_x;
                hero.y = last_y;
            }
        }
        else if(map_check[hero.y][hero.x]=='9'&& can_grab==1){
            mvprintw(0,0,"KEY GRABED!                                              ");
            hero.has_key=1;
            map_check[hero.y][hero.x]='.';
            map->map[hero.y][hero.x]= '.';
        }
        else if(map_check[hero.y][hero.x]=='!'){
            map_check[hero.y][hero.x]='3';
            map->map[hero.y][hero.x]= '3';
        }
        else if(map_check[hero.y][hero.x]=='?'){
            map_check[hero.y][hero.x]='3';
            map->map[hero.y][hero.x]= '3';
        }
        else if(map_check[hero.y][hero.x]=='8'){
            map_check[hero.y][hero.x]='t';
            map->map[hero.y][hero.x]= 't';  
            mvprintw(0,0,"trap hurts you                                              ");          
            hero.health-=1;
        }
        else if(map_check[hero.y][hero.x]=='T'){
            
            attron(COLOR_PAIR(3));
            attron(A_BOLD);
            mvprintw((LINES/2)-3,(COLS/2)-31,"🌟🌟CONGRAJULATOIN🌟🌟");
            mvprintw((LINES/2)-2,(COLS/2)-42,"🌟🌟YOU REACHED TREASURE ROOM🌟🌟");
            mvprintw((LINES/2)-1,(COLS/2)-42,"🌟🌟YOU WIN🌟🌟");
            attroff(COLOR_PAIR(3));
            attroff(A_BOLD);
            win=1;
            napms(2000);
            return 'q';
        }
        else if(map_check[hero.y][hero.x]=='<'){
            mvprintw(0,0,"Click right key...                                              ");
            timeout(-1);
            int change_floor=getch();
            switch(change_floor){
                case KEY_RIGHT:
                    if (current_floor < 4) { // اگر هنوز در پایین‌ترین طبقه نیستیم
                        current_floor++;
                        printf("Moved to floor: %d\n", current_floor);
                    } else {
                        printf("You are already at the lowest floor!\n");
                    }
                    break;
                case KEY_LEFT:
                    if (current_floor > 1) { // اگر هنوز در بالاترین طبقه نیستیم
                        current_floor--;
                        printf("Moved to floor: %d\n", current_floor);
                    } else {
                        printf("You are already at the top floor!\n");
                    }
                    break;
                default:
                    break;
            }  
            getch();
            
            if(current_floor==1){
                display_visible_map(map_ptr_floor1, *visible_ptr_floor1);
                map_ptr=map_ptr_floor1;
                map_check_ptr=map_check_ptr_floor1;
                visible_ptr=visible_ptr_floor1;
            }      
            else if(current_floor==2){                
                display_visible_map(&map_floor_2,visible_floor_2);
                map_ptr=&map_floor_2;
                map_check_ptr=&map_check_floor_2;
                visible_ptr=&visible_floor_2;
            }
            else if(current_floor==3){
                display_visible_map(&map_floor_3,visible_floor_3);
                map_ptr=&map_floor_3;
                map_check_ptr=&map_check_floor_3;
                visible_ptr=&visible_floor_3;
            }
            else if(current_floor==4){
                display_visible_map(&map_floor_4,visible_floor_4);
                map_ptr=&map_floor_4;
                map_check_ptr=&map_check_floor_4;
                visible_ptr=&visible_floor_4;
            }
        }
        else if(map_check[hero.y][hero.x]=='F'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            if (hero.food_count < MAX_FOOD_INVENTORY){
                hero.inventory[0]++;
                hero.food_count++;
                mvprintw(0,0,"Food grabed!                                                  ");
                map_check[hero.y][hero.x]='.';
                map->map[hero.y][hero.x] = '.';
            } else {
                mvprintw(0,0,"You can't grab more!                                                  ");
                map_check[hero.y][hero.x]='F';
                map->map[hero.y][hero.x] = 'F';
            }
            getch();
        }
        else if(map_check[hero.y][hero.x]=='$'){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.gold+=rand()%3+5; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "You have %d gold!",hero.gold);
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='g'){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.gold+=rand()%3+10; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "You have %d gold!",hero.gold);
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='C'&&can_grab==1){
            timeout(-1);
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            } // جای جدید بازیکن
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            hero.weapon[0]+=1; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "Mace grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='e'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.weapon[1]+=10; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "Dagger grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='N'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.weapon[2]+=8; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "Magic Wand grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='A'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.weapon[3]+=20; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "Normal Arrow grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='W'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.weapon[4]+=1; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "Sword grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='^'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.weapon[1]+=1; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "1 Dagger grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='*'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.weapon[2]+=1; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "1 Magic Wand grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='%'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.weapon[3]+=1; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "1 Normal Arrow grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='s'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.spell[1]+=1; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "speed spell grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='d'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.spell[2]+=1; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "damage spell grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        else if(map_check[hero.y][hero.x]=='h'&&can_grab==1){
            timeout(-1);
            map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
            if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
            hero.spell[0]+=1; 
            map->map[hero.y][hero.x]='.';
            map_check[hero.y][hero.x]='.';
            mvprintw(0, 0, "health spell grabed!                      ");
            getch();
            timeout(10);
            mvprintw(0, 0, "                                  ");
        }
        map->map[hero.y][hero.x] = 'H'; // جای جدید بازیکن
        if(strcmp(settings.main_color,"Red")==0){
                attron(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attron(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attron(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attron(COLOR_PAIR(4));
            }
            mvprintw(hero.y + 1, hero.x, "%c", map->map[hero.y][hero.x]);
            if(strcmp(settings.main_color,"Red")==0){
                attroff(COLOR_PAIR(1));
            }
            if(strcmp(settings.main_color,"Green")==0){
                attroff(COLOR_PAIR(2));
            }
            if(strcmp(settings.main_color,"Blue")==0){
                attroff(COLOR_PAIR(3));
            }
            if(strcmp(settings.main_color,"Yellow")==0){
                attroff(COLOR_PAIR(4));
            }
        visible[hero.y][hero.x]=1; 
    }
    return ch;
}
void move_between_floors(int direction,char*username) {
    if (direction == 1) { // حرکت به پایین (Right Arrow)
        if (current_floor < 4) { // اگر هنوز در پایین‌ترین طبقه نیستیم
            current_floor++;
            printf("Moved to floor: %d\n", current_floor);
        } else {
            printf("You are already at the lowest floor!\n");
        }
    } else if (direction == -1) { // حرکت به بالا (Left Arrow)
        if (current_floor > 1) { // اگر هنوز در بالاترین طبقه نیستیم
            current_floor--;
            printf("Moved to floor: %d\n", current_floor);
        } else {
            printf("You are already at the top floor!\n");
        }
    }
}
void print_colored_massage(char *message, int color_pair){
    //init_color(COLOR_ORANGE, 500, 270, 0);
    init_color(COLOR_yellow, 1000,900, 0);
    init_pair(2, COLOR_yellow, COLOR_BLACK); // رنگ هشدار زرد
    init_pair(1, COLOR_ORANGE, COLOR_BLACK); 
    init_pair(3, COLOR_GREEN, COLOR_BLACK);  // رنگ موفقیت سبز
    attron(COLOR_PAIR(2));
    mvprintw(0,17,"%s", message);
    mvprintw(1,0, "%d",color_pair);
    attroff(COLOR_PAIR(2));
    refresh();
}
int is_valid_move(Map *map, int new_y, int new_x) {
    char target = map->map[new_y][new_x];
    // اگر مقصد دیوار، ستون یا خارج از نقشه بود، حرکت نامعتبر است
    return (target != '|' && target != '-' && target != 'O' && target != ' '&& target != 'D'&& target != 'E'&& target != 'G'&& target != 'S'&& target != 'U');
}
int which_room(Map *map, int x, int y) {
    // پیمایش تمام اتاق‌ها
    for (int i = 0; i < map->room_count; i++) {
        Room *room = &map->rooms[i];

        // بررسی اینکه آیا مختصات در محدوده اتاق قرار دارد
        if (x >= room->x && x < room->x + room->width &&
            y >= room->y && y < room->y + room->height) {
            return i; // شماره اتاق را برگردان
        }
    }
    return -1; // اگر مختصات به هیچ اتاقی تعلق نداشت
}
void show_rooms(Map *map,int x,int y){
    return;
}
void show_code_temporarily(WINDOW *win, int x, int y,char *code) {
    mvwprintw(win, y, x, "Code: %s", code); // نمایش رمز
    wrefresh(win);
    sleep(30); // منتظر ماندن به مدت 30 ثانیه
    mvwprintw(win, y, x, "             "); // پاک کردن رمز
    wrefresh(win);
}
void display_visible_map(Map*map, int** visible) {
    init_pair(1,COLOR_RED,COLOR_BLACK);
    init_pair(2,COLOR_GREEN,COLOR_BLACK);
    init_pair(3,COLOR_BLUE,COLOR_BLACK);
    init_pair(4,COLOR_YELLOW,COLOR_BLACK);
    for (int y = 0; y < LINES; y++) {
        for (int x = 0; x < COLS; x++) {
            if (visible[y][x]) {
                if(map->map[y][x]=='9'){
                    const char* key="△";
                    attron(COLOR_PAIR(4));
                    mvprintw(y + 1, x, "%s",key);                
                    attroff(COLOR_PAIR(4));
                }else if(map->map[y][x]=='8'){
                    const char* key="∧";
                    attron(COLOR_PAIR(1));
                    mvprintw(y + 1, x, "%s",key); 
                    attroff(COLOR_PAIR(1));
                }else if(map->map[y][x]=='1'){
                    const char* key="@";
                    attron(COLOR_PAIR(1));
                    mvprintw(y + 1, x, "%s",key);
                    attroff(COLOR_PAIR(1)); 
                }else if(map->map[y][x]=='g'){
                    const char* black_gold="€";
                    mvprintw(y + 1, x, "%s",black_gold); 
                }else if(map->map[y][x]=='C'){
                    const char* Mace="⍑";
                    mvprintw(y + 1, x, "%s",Mace); 
                }else if(map->map[y][x]=='e'){
                    const char* Dagger="ⴕ";
                    mvprintw(y + 1, x, "%s",Dagger); 
                }else if(map->map[y][x]=='N'){
                    const char* Magic="⧙";
                    mvprintw(y + 1, x, "%s",Magic); 
                }else if(map->map[y][x]=='A'){
                    const char* Arrow="➤";
                    mvprintw(y + 1, x, "%s",Arrow); 
                }else if(map->map[y][x]=='^'){
                    const char* Dagger="ⴕ";
                    mvprintw(y + 1, x, "%s",Dagger); 
                }else if(map->map[y][x]=='*'){
                    const char* Magic="⧙";
                    mvprintw(y + 1, x, "%s",Magic); 
                }else if(map->map[y][x]=='%'){
                    const char* Arrow="➤";
                    mvprintw(y + 1, x, "%s",Arrow); 
                }else if(map->map[y][x]=='W'){
                    const char* Sword="⟆";
                    mvprintw(y + 1, x, "%s",Sword); 
                }else if(map->map[y][x]=='H'){
                    if(strcmp(settings.main_color,"Red")==0){
                        attron(COLOR_PAIR(1));
                    }
                    if(strcmp(settings.main_color,"Green")==0){
                        attron(COLOR_PAIR(2));
                    }
                    if(strcmp(settings.main_color,"Blue")==0){
                        attron(COLOR_PAIR(3));
                    }
                    if(strcmp(settings.main_color,"Yellow")==0){
                        attron(COLOR_PAIR(4));
                    }
                    mvprintw(y + 1, x, "H");
                    if(strcmp(settings.main_color,"Red")==0){
                        attroff(COLOR_PAIR(1));
                    }
                    if(strcmp(settings.main_color,"Green")==0){
                        attroff(COLOR_PAIR(2));
                    }
                    if(strcmp(settings.main_color,"Blue")==0){
                        attroff(COLOR_PAIR(3));
                    }
                    if(strcmp(settings.main_color,"Yellow")==0){
                        attroff(COLOR_PAIR(4));
                    } 
                }else if(map->map[y][x]=='2'){
                    attron(COLOR_PAIR(2));
                    mvprintw(y + 1, x, "@");
                    attroff(COLOR_PAIR(2));
                }else if(map->map[y][x]=='1'){
                    attron(COLOR_PAIR(1));
                    mvprintw(y + 1, x, "@");
                    attroff(COLOR_PAIR(1));
                }else if(map->map[y][x]=='&'){
                    attron(COLOR_PAIR(3));
                    mvprintw(y + 1, x, "&");
                    attroff(COLOR_PAIR(3));
                }else
                    mvprintw(y + 1, x, "%c", map->map[y][x]);
            } else {
                mvaddch(y+1, x, ' '); // خانه‌های مخفی
            }
        }
    }
}
void play_music()
{
	char command[256];

	kill_music();
	if (settings.selected_music == 0)
	{
		return;
	}
	if (settings.selected_music == 1)
	{
		snprintf(command, sizeof(command), "mpg123 1.mp3 > /dev/null 2>&1 &");
	}
	else if (settings.selected_music == 2)
	{
		snprintf(command, sizeof(command), "mpg123 2.mp3 > /dev/null 2>&1 &");
	}
	else if (settings.selected_music == 3)
	{
		snprintf(command, sizeof(command), "mpg123 3.mp3 > /dev/null 2>&1 &");
	}
	system(command);
}
void kill_music()
{
	char command[256];
	snprintf(command, sizeof(command), "pkill mpg123");
	system(command);
}
void* check_code_timer(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex); // قفل کردن برای دسترسی امن به متغیرها
        if (code_shown && difftime(time(NULL), code_start_time) >= 30) {
            mvprintw(12, 0, "              "); // پاک کردن پیام
            code_shown = false; // غیرفعال کردن نمایش رمز
        }
        pthread_mutex_unlock(&mutex); // آزاد کردن قفل
        usleep(100000); // خواب 100 میلی‌ثانیه برای کاهش مصرف پردازنده
    }
    return NULL;
}
void* check_code_timer_heal(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex_heal); // قفل کردن برای دسترسی امن به متغیرها
        if (difftime(time(NULL), code_start_time_heal) >= 10 && hero.hunger<10) {
            if(hero.health<10){
                hero.health++;
                if(health_spel>0){
                    hero.health++;
                }
            } // غیرفعال کردن نمایش رمز
            //mvprintw(1, 0, "Your health: %d              ",hero.health); // پاک کردن پیام
        }
        pthread_mutex_unlock(&mutex_heal); // آزاد کردن قفل
        usleep(100000); // خواب 100 میلی‌ثانیه برای کاهش مصرف پردازنده
    }
    return NULL;
}
void* check_code_timer_hunger(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex_hunger); // قفل کردن برای دسترسی امن به متغیرها
        if (difftime(time(NULL), code_start_time_hunger) >= 10 && hero.hunger<10) {
            if(hero.hunger<10){
                hero.hunger++;
            } // غیرفعال کردن نمایش رمز
            //mvprintw(1, 0, "Your hunger: %d              ",hero.hunger); // پاک کردن پیام
        }
        pthread_mutex_unlock(&mutex_hunger); // آزاد کردن قفل
        usleep(100000); // خواب 100 میلی‌ثانیه برای کاهش مصرف پردازنده
    }
    return NULL;
}
void add_food_to_hero() {
    timeout(-1);
    if (hero.food_count < MAX_FOOD_INVENTORY){
        hero.inventory[0]++;
        hero.food_count++;
        mvprintw(0,0,"Food grabed!                                                  ");
    } else {
        mvprintw(0,0,"You can't grab more!                                                  ");
    }
    getch();
    timeout(10);
}
void food_menu(){
    // Show options for new game or continue game
    clear();
    WINDOW *menu_win;
    int highlight = 1;
    int choice = 0;
    int c;

    char *choices[] = {
        "normal",
        "special",
        "magic",
        "poisoned",
        "Exit"
    };
    int n_choices = sizeof(choices) / sizeof(char *);
    menu_win = newwin(10, 40, (LINES - 10) / 2, (COLS - 40) / 2);
    keypad(menu_win, TRUE);
    mvprintw(0, 0, "Choose witch to eat!  hungr:                   ");
    for (int i = 0; i <= hero.hunger; i++)
    {
        mvprintw(0, 29+i, "#");
    }
    refresh();
    print_menu(menu_win, highlight, choices, n_choices);

    while (1) {
        c = wgetch(menu_win);
        switch (c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;
            case 10: // Enter key
                choice = highlight;
                break;
            default:
                refresh();
                break;
        }
        print_menu(menu_win, highlight, choices, n_choices);
        if (choice != 0)
            break;
    }
    clrtoeol();
    refresh();
    delwin(menu_win);
    timeout(-1);
    // Handle the user's choice
    switch (choice) {
        case 1:
            // Start a new game
            if(hero.inventory[0]==0){
                mvprintw(0, 0, "You don't have normal food        ");
                getch();
                food_menu();
            }
            else{
                mvprintw(0, 0, "You eat one normal food         ");
                hero.inventory[0]-=1;
                hero.food_count-=1;
                hero.hunger=0;
                hero.health=10;
                getch();
                food_menu();

            }
            break;
        case 2:
            if(hero.inventory[1]==0){
                mvprintw(0, 0, "You don't have special food         ");
                getch();
                food_menu();
            }
            else{
                mvprintw(0, 0, "You eat one special food             ");
                hero.inventory[1]-=1;
                hero.food_count-=1;
                getch();
                food_menu();

            }
            break;
        case 3:
            if(hero.inventory[2]==0){
                mvprintw(0, 0, "You don't have magic food         ");
                getch();
                food_menu();
            }
            else{
                mvprintw(0, 0, "You eat one magic food            ");
                hero.inventory[2]-=1;
                hero.food_count-=1;
                getch();
                food_menu();

            }
            break;
        case 4:
            if(hero.inventory[3]==0){
                mvprintw(0, 0, "You don't have poisoned food          ");
                getch();
                food_menu();
            }
            else{
                mvprintw(0, 0, "You eat one poisoned food         ");
                hero.inventory[3]-=1;
                hero.food_count-=1;
                getch();
                food_menu();

            }
            break;     
        case 5:
            clear();
            break;
    }
    mvprintw(0, 0, "                                                   ");
}

void save_matrices(char *username, int** matrices,int i,int j) {
    char filename[60];
    snprintf(filename, sizeof(filename), "%s_game.txt", username);
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Eror...");
        return;
    }
    for (int r = 0; r < LINES; r++) {
        for (int c = 0; c < COLS; c++) {
            fprintf(file, "%d ", matrices[r][c]);
        }
        fprintf(file, "\n");  // رفتن به خط بعدی برای سطر بعدی
    }
    fprintf(file, "===\n");  // جداکننده بین ماتریس‌ها

    fclose(file);
}

// تابع خواندن ماتریس‌ها از فایل
void load_matrices(char *username, int** matrices,int i,int j,FILE *file) {
    for (int r = 0; r < LINES; r++) {
        for (int c = 0; c < COLS; c++) {
            if (fscanf(file, "%d ",&matrices[r][c]) != 1) {
                break;  // در صورت خطا، از حلقه خارج شو
            }
        }
    }
    char separator[10];  // خواندن جداکننده "==="
    fscanf(file, "%s", separator);
}
void save_hero(FILE *file, Hero *hero) {
    fprintf(file, "Hero:\n");
    fprintf(file, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            hero->x, hero->y,
            hero->health, hero->hunger, hero->gold,
            hero->inventory[0], hero->inventory[1],
            hero->inventory[2], hero->inventory[3],
            hero->weapon[0], hero->weapon[1],
            hero->weapon[2], hero->weapon[3],
            hero->weapon[4],hero->using_weapon, hero->food_count,hero->spell[0],
            hero->spell[1],hero->spell[2],hero->has_key,hero->has_broken_key);
    fprintf(file, "current floor:%d\n",current_floor);
}
void load_hero(FILE *file, Hero *hero) {
    fscanf(file, "Hero:\n");
    fscanf(file, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            &hero->x, &hero->y,
            &hero->health, &hero->hunger, &hero->gold,
            &hero->inventory[0], &hero->inventory[1],
            &hero->inventory[2], &hero->inventory[3],
            &hero->weapon[0], &hero->weapon[1],
            &hero->weapon[2], &hero->weapon[3],
            &hero->weapon[4],&hero->using_weapon, &hero->food_count,&hero->spell[0],
            &hero->spell[1],&hero->spell[2],&hero->has_key,&hero->has_broken_key);
    fscanf(file, "current floor:%d\n",&current_floor);
}
void save_room(FILE *file, Room *room) {
    fprintf(file, "Room:\n");
    fprintf(file, "%d %d %d %d ", room->x, room->y, room->width, room->height);
    fprintf(file, "%d %d %s %d %d %d %d %d %d %d\n",
            room->has_secret_door,
            room->has_password_door,
            room->password,
            room->has_master_key,
            room->master_key_used,
            room->opend,
            room->is_regular,
            room->is_treasure,
            room->is_enchant,
            room->is_nightmare
    );
}
void save_all_rooms(FILE *file, Map *map) {
    fprintf(file, "Total Rooms: %d\n", map->room_count); // ذخیره تعداد اتاق‌ها
    for (int i = 0; i < map->room_count; i++) {
        save_room(file, &map->rooms[i]);
    }
}
void load_room(FILE *file, Room *room) {
    fscanf(file, "Room:\n");
    fscanf(file, "%d %d %d %d ", &room->x, &room->y, &room->width, &room->height);
    fscanf(file, "%d %d %6s %d %d %d %d %d %d %d\n",
           &room->has_secret_door,
           &room->has_password_door,
           room->password,
           &room->has_master_key,
           &room->master_key_used,
           &room->opend,
           &room->is_regular,
           &room->is_treasure,
           &room->is_enchant,
           &room->is_nightmare
    );
}
void load_all_rooms(FILE *file, Map *map) {
    int total_rooms;
    fscanf(file, "Total Rooms: %d\n", &total_rooms);
    map->room_count = total_rooms;

    for (int i = 0; i < total_rooms; i++) {
        load_room(file, &map->rooms[i]);
    }
}
double get_elapsed_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}
void enemis_move(Map *map,int***map_check_ptr,int***visible_ptr,char* username,char enemy,int room_num,int i, int j){
    int dx = hero.x - j;
    int dy = hero.y - i;
    if (dx > 1) {
        if(map_check[i][j+1]=='.'){
            map->map[i][j]='.';
            map->map[i][j+1]=enemy;
            map_check[i][j]='.';
            map_check[i][j+1]=enemy;
            chase_steps_deamon--;
        }else if(dy > 1){
            if(map_check[i+1][j]=='.'){
                map->map[i][j]='.';
                map->map[i+1][j]=enemy;
                map_check[i][j]='.';
                map_check[i+1][j]=enemy;
                chase_steps_deamon--;
            }
        }else if(dy < -1){
            if(map_check[i-1][j]=='.'){
                map->map[i][j]='.';
                map->map[i-1][j]=enemy;
                map_check[i][j]='.';
                map_check[i-1][j]=enemy;
                chase_steps_deamon--;
            }
        }
    }else if (dx < -1){
        if(map_check[i][j-1]=='.'){
            map->map[i][j]='.';
            map->map[i][j-1]=enemy;
            map_check[i][j]='.';
            map_check[i][j-1]=enemy;
            chase_steps_deamon--;
        }else if(dy > 1){
            if(map_check[i+1][j]=='.'){
                map->map[i][j]='.';
                map->map[i+1][j]=enemy;
                map_check[i][j]='.';
                map_check[i+1][j]=enemy;
                chase_steps_deamon--;
            }
        }else if(dy < -1){
            if(map_check[i-1][j]=='.'){
                map->map[i][j]='.';
                map->map[i-1][j]=enemy;
                map_check[i][j]='.';
                map_check[i-1][j]=enemy;
                chase_steps_deamon--;
            }  
        }
    }
    else if (dy > 1){
        if(map_check[i+1][j]=='.'){
            map->map[i][j]='.';
            map->map[i+1][j]=enemy;
            map_check[i][j]='.';
            map_check[i+1][j]=enemy;
            chase_steps_deamon--;
        }else if(dx > 1){
            if(map_check[i][j+1]=='.'){
                map->map[i][j]='.';
                map->map[i][j+1]=enemy;
                map_check[i][j]='.';
                map_check[i][j+1]=enemy;
                chase_steps_deamon--;
            }
        }else if(dx < -1){
            if(map_check[i][j-1]=='.'){
                map->map[i][j]='.';
                map->map[i][j-1]=enemy;
                map_check[i][j]='.';
                map_check[i][j-1]=enemy;
                chase_steps_deamon--;
            }  
        }
    }else if (dy < -1){
        if(map_check[i-1][j]=='.'){
            map->map[i][j]='.';
            map->map[i-1][j]=enemy;
            map_check[i][j]='.';
            map_check[i-1][j]=enemy;
            chase_steps_deamon--;
        }else if(dx > 1){
            if(map_check[i][j+1]=='.'){
                map->map[i][j]='.';
                map->map[i][j+1]=enemy;
                map_check[i][j]='.';
                map_check[i][j+1]=enemy;
                chase_steps_deamon--;
            }
        }else if(dx < -1){
            if(map_check[i][j-1]=='.'){
                map->map[i][j]='.';
                map->map[i][j-1]=enemy;
                map_check[i][j]='.';
                map_check[i][j-1]=enemy;
                chase_steps_deamon--;
            }  
        }
    } 
}
void update_player_score(char *filename, char *target_username, int score_change, int gold_change) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening leaderboard file!\n");
        return;
    }

    Player players[100]; // حداکثر ۱۰۰ بازیکن
    int player_count = 0;
    char line[256];

    // خواندن اطلاعات بازیکنان از فایل
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%d,%d,%d,%ld", 
               players[player_count].username, 
               &players[player_count].total_score, 
               &players[player_count].total_gold, 
               &players[player_count].games_played, 
               &players[player_count].first_game_time);
        player_count++;
    }
    fclose(file);

    // جستجوی کاربر و تغییر اطلاعات
    int found = 0;
    for (int i = 0; i < player_count; i++) {
        if (strcmp(players[i].username, target_username) == 0) {
            players[i].total_score += score_change;  // افزایش امتیاز
            players[i].total_gold += gold_change;// افزایش تعداد بازی‌ها
            found = 1;
            break;
        }
    }

    if (!found) {
        return;
    }

    // بازنویسی فایل با اطلاعات جدید
    file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening leaderboard file for writing!\n");
        return;
    }

    for (int i = 0; i < player_count; i++) {
        fprintf(file, "%s,%d,%d,%d,%ld\n", 
                players[i].username, 
                players[i].total_score, 
                players[i].total_gold, 
                players[i].games_played, 
                players[i].first_game_time);
    }
    fclose(file);
}
/*
void spawn_enemies(EnemyList *list, int map_width, int map_height) {
    if (list->count >= MAX_ENEMIES) return; // اگر به حداکثر رسیده
    
    // احتمال 30% برای ایجاد دشمن جدید
    if ((rand() % 100) < SPAWN_RATE) {
        Enemy new_enemy = create_random_enemy(map_width, map_height);
        list->enemies[list->count] = new_enemy;
        list->count++;
    }
}
Enemy create_random_enemy(int map_width, int map_height) {
    Enemy enemy;
    
    // انتخاب تصادفی نوع دشمن
    enemy.type = rand() % 5; // 0 تا 4
    
    // تعیین نماد و ویژگی‌ها بر اساس نوع
    switch (enemy.type) {
        case DEAMON:
            enemy.symbol = 'D';
            enemy.health = 8;
            break;
        case FIRE_BREATHING_MONSTER:
            enemy.symbol = 'F';
            enemy.health = 10;
            break;
        case GIANT:
            enemy.symbol = 'G';
            enemy.health = 15;
            break;
        case SNAKE:
            enemy.symbol = 'S';
            enemy.health = 20;
            break;
        case UNDEAD:
            enemy.symbol = 'U';
            enemy.health = 30;
            break;
    }
    
    // موقعیت تصادفی در محدوده نقشه
    enemy.x = rand() % map_width;
    enemy.y = rand() % map_height;
    enemy.is_chasing = false;
    enemy.chase_steps = 0;
    
    return enemy;
}*/
