#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <wchar.h>

#define MAX_MUSIC_TRACKS 10
#define MAX_ROOMS 6
#define ROOM_MIN_WIDTH 6
#define ROOM_MIN_HEIGHT 6

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
Settings settings;
typedef struct {
    int x, y, width, height; // موقعیت و اندازه اتاق
    int has_secret_door;     // آیا اتاق دارای در مخفی است؟
    int has_password_door;   // آیا اتاق دارای در رمزدار است؟
    char password[6];        // رمز در صورتی که رمزدار باشد
    int has_master_key;      // آیا کلید خاص در اتاق وجود دارد؟
    int master_key_used;     // آیا این کلید استفاده شده است؟
} Room;

typedef struct {
    Room rooms[MAX_ROOMS];
    int room_count;
    int **map;
} Map;

// Function declarations
void create_room(Room *room, int max_width, int max_height);
void place_rooms(Map *map, int max_width, int max_height);
void connect_rooms(Map *map);
void add_pillars_stair_traps(Map *map);
void generate_map(Map *map, int max_width, int max_height);
void print_map(Map *map, int max_width, int max_height);
void create_user();
void generate_random_password(char *password, int length);
void print_menu(WINDOW *menu_win, int highlight, char **choices, int n_choices);
int start_menu();
int is_username_taken(const char *username);
void user_entrance_menu();
void reset_password_help();
void before_game_menu(char* username);
void start_new_game(char *username);
void continue_game(char *username);
void view_leaderboard(char *username);

void display_settings_menu(char *username);
void change_difficulty();
void change_color();
void select_music();
void save_settings(const char *username);
void load_settings(const char *username);

void secret_room(Map *map,int i);
void password_room(Map *map,int i);
/*void load_music(settings);*/
void print_settings_menu(WINDOW *menu_win, int highlight, char **choices, int n_choices);
int main() {
    initscr();
    clear();
    noecho();
    cbreak();  // Line buffering disabled. pass on everything
    curs_set(0);
    settings.difficulty=1;
    strcpy(settings.main_color,"Red");
    //strcpy (settings.music,"Track1");
    settings.selected_music=1;
    int choice;
    while((choice = start_menu()) != 3) {
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
                getch();
                break;
            case 3:
                clear();
                endwin();
                break;
            default:
                printw("Invalid choice\n");
                getch();
                break;
        }
    }

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

    if (strstr(email, "@") == NULL || strstr(email, ".") == NULL) {
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
    fprintf(file, "%s,%d,%d,%d,%d\n", username,0,0,0,0);
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
void before_game_menu(char* username){
/*    mvwprintw(input_win, 5, 1, "Login successful");
    wrefresh(input_win);
    getch();
    delwin(input_win);
*/
    // Show options for new game or continue game
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
            view_leaderboard(username);
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
    // Initialize a new game and save the initial state to the user's file
    char filename[60];
    snprintf(filename, sizeof(filename), "%s_game.txt", username);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printw("Error creating game file");
        getch();
        return;
    }

    // Save the initial game state
    fprintf(file, "New game started\n");
    fclose(file);
    int max_width, max_height;
    getmaxyx(stdscr, max_height, max_width);
    Map map;
    map.map = (int **)malloc(max_height * sizeof(int *));
    for (int i = 0; i < max_height; i++) {
        map.map[i] = (int *)malloc(max_width * sizeof(int));
    }

    srand(time(NULL));
    generate_map(&map, max_width, max_height);
    print_map(&map, max_width, max_height);

    for (int i = 0; i < max_height; i++) {
        free(map.map[i]);
    }
    free(map.map);
    getch();
    before_game_menu(username);
    refresh();
}
void continue_game(char *username) {
    // Load the previous game state from the user's file
    char filename[60];
    snprintf(filename, sizeof(filename), "%s_game.txt", username);

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printw("No previous game found. Starting a new game.");
        start_new_game(username);
        return;
    }

    // Read and print the previous game state
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        printw("%s", line);
    }
    fclose(file);

    refresh();
    getch();
}
void view_leaderboard(char *username) {
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
        for (int j = i + 1; j < player_count; j++) {
            if (players[i].total_score < players[j].total_score) {
                Player temp = players[i];
                players[i] = players[j];
                players[j] = temp;
            }
        }
    }

    // نمایش ۵ نفر اول در جدول امتیازات
    WINDOW *leaderboard_win = newwin(20, 80, (LINES - 20) / 2, (COLS - 80) / 2);
    box(leaderboard_win, 0, 0);
    mvwprintw(leaderboard_win, 1, 2, "Rank   Username   Total Score   Total Gold   Games Played   Time Elapsed");
    mvwprintw(leaderboard_win, 2, 1, "-----------------------------------------------------------------------");

    for (int i = 0; i < player_count && i < 5; i++) {
        // محاسبه زمان سپری شده از اولین بازی
        time_t current_time = time(NULL);
        double time_elapsed = difftime(current_time, players[i].first_game_time) / (60 * 60 * 24); // تبدیل به روز

        if (strcmp(username, players[i].username) == 0) {
            wattron(leaderboard_win, A_BOLD);
        }

        mvwprintw(leaderboard_win, i + 3, 2, "%-5d  %-10s  %-12d  %-10d  %-13d  %.1f days", 
                  i + 1, players[i].username, players[i].total_score, 
                  players[i].total_gold, players[i].games_played, time_elapsed);

        if (strcmp(username, players[i].username) == 0) {
            wattroff(leaderboard_win, A_BOLD);
        }
    }

    wrefresh(leaderboard_win);
    getch();
    delwin(leaderboard_win);
    clear();
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
            break;
        case 2:
            clear();
            change_color();
            break;
        case 3:
            clear();
            select_music();
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

    music_win = newwin(10, 40, (LINES/2)-12, (COLS/2)-3);
    keypad(music_win, TRUE);
    mvprintw(0, 0, "Use arrow keys to navigate and Enter to select");
    refresh();
    while(1) {
        for (int i = 0; i < MAX_MUSIC_TRACKS; ++i) {
            if (highlight == i + 1) {
                wattron(music_win, A_REVERSE);
                mvwprintw(music_win, i + 2, 2, "%s", settings.music[i]);
                wattroff(music_win, A_REVERSE);
            } else {
                mvwprintw(music_win, i + 2, 2, "%s", settings.music[i]);
            }
        }
        wrefresh(music_win);

        c = wgetch(music_win);
        switch(c) {
            case KEY_UP:
                if (highlight == 1)
                    highlight = MAX_MUSIC_TRACKS;
                else
                    --highlight;
                break;
            case KEY_DOWN:
                if (highlight == MAX_MUSIC_TRACKS)
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

void load_settings(const char *username) {
    FILE *file = fopen(username, "r");
    if (file == NULL) {
        // Default settings
        settings.difficulty = 1;
        strcpy(settings.main_color, "Red");
        settings.selected_music = 0;
        //load_music(settings);
        return;
    }
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
    } else if (rand() % 3 == 0) { // 20% احتمال برای در رمزدار
        room->has_password_door = 1;
        snprintf(room->password, 6, "%04d", rand() % 10000); // تولید رمز 4 رقمی
    }
}

void place_rooms(Map *map, int max_width, int max_height) {
    map->room_count = 0;
    while (map->room_count < MAX_ROOMS) {
        Room new_room;
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
            // اضافه کردن ورودی اتاق در وسط هر دیوار (بدون گوشه‌ها)
            /*map->map[new_room.y + new_room.height / 2][new_room.x] = '+'; // ورودی در دیوار چپ
            map->map[new_room.y][new_room.x + new_room.width / 2] = '+'; // ورودی در دیوار بالا
            map->map[new_room.y + new_room.height / 2][new_room.x + new_room.width - 1] = '+'; // ورودی در دیوار راست
            map->map[new_room.y + new_room.height - 1][new_room.x + new_room.width / 2] = '+'; // ورودی در دیوار پایین
        */
       }
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
        
        // بررسی اینکه فاصله بین اتاق‌ها حداقل ۱۰ واحد باشد
        int distance = abs(x1 - x2) + abs(y1 - y2);
        if (distance < 10) {
            // در اینجا می‌توانید کدی اضافه کنید تا اتاق‌های بعدی که فاصله‌ی مناسب دارند انتخاب شوند.
            continue;
        }

        // مسیر را از نقطه شروع به سمت نقطه مقصد رسم می‌کنیم
        while (x1 != x2) {
            if (map->map[y1][x1] == '.') {
                map->map[y1][x1] = '+';
            } else {
                map->map[y1][x1] = '#';
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
            }
            y1 += (y2 > y1) ? 1 : -1;
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
            map->map[room->y][x] = '.';
            return;
        }
        if( map->map[room->y + room->height - 1][x] == '+'){
            map->map[room->y + room->height - 1][x] = '.';
            return;
        }
    }
    for (int y = room->y; y < room->y + room->height; y++) {
        if(map->map[y][room->x] == '+'){
            map->map[y][room->x] = '.';
            return;
        }
        if(map->map[y][room->x + room->width - 1] == '+'){
            map->map[y][room->x + room->width - 1] = '.';
            return;
        }
    }
}
void password_room(Map *map,int i){
    Room *room = &map->rooms[i];
    int px = room->x+2 + rand() % (room->width-4);
    int py = room->y+2 + rand() % (room->height-4);
    start_color();        // فعال کردن رنگ‌ها
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    map->map[py][px] = '&'; // دکمه تولید رمز
    /*if (player_x == password_door_x && player_y == password_door_y) {
        char input_password[5];
        mvprintw(0, 0, "Enter the password: ");
        echo();
        getstr(input_password);
        noecho();
        if (strcmp(input_password, current_room->password) == 0) {
            mvprintw(0, 0, "Door unlocked!");
            map->map[password_door_y][password_door_x] = '+'; // باز کردن در
        } else {
            mvprintw(0, 0, "Wrong password!");
        }
    }*/
    for (int x = room->x; x < room->x + room->width; x++) {
        if(map->map[room->y][x] == '+'){
            map->map[room->y][x] = '@';
            return;
        }
        if( map->map[room->y + room->height - 1][x] == '+'){
            map->map[room->y + room->height - 1][x] = '@';
            return;
        }
    }
    for (int y = room->y; y < room->y + room->height; y++) {
        if(map->map[y][room->x] == '+'){
            map->map[y][room->x] = '@';
            return;
        }
        if(map->map[y][room->x + room->width - 1] == '+'){
            map->map[y][room->x + room->width - 1] = '@';
            return;
        }
    }
}
void add_pillars_stair_traps(Map *map) {
    for (int i = 0; i < map->room_count; i++) {
        Room *room = &map->rooms[i];
        int pillars = (room->width - 2) * (room->height - 2) / 16; // تعداد ستون‌ها
        int traps= rand()%2;
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
                map->map[py][px] = '^'; // ستون
            }
        }
    }
    Room *room = &map->rooms[0];
    map->map[3][3] = '<'; // ستون

}

void generate_map(Map *map, int max_width, int max_height) {
    for (int y = 0; y < max_height; y++) {
        for (int x = 0; x < max_width; x++) {
            map->map[y][x] = ' ';
        }
    }
    place_rooms(map, max_width, max_height);
    connect_rooms(map);
    add_pillars_stair_traps(map);
    int have_secrest ,have_password =0;
    for (int i = 0; i < map->room_count; i++) {
        Room *room = &map->rooms[i];
        for (int x = room->x; x < room->x + room->width; x++) {
            if(map->map[room->y][x] != '+')
                map->map[room->y][x] = '-';
            if( map->map[room->y + room->height - 1][x] != '+')
                map->map[room->y + room->height - 1][x] = '-';
        }
        for (int y = room->y; y < room->y + room->height; y++) {
            if(map->map[y][room->x] != '+')
                map->map[y][room->x] = '|';
            if(map->map[y][room->x + room->width - 1] != '+')
                map->map[y][room->x + room->width - 1] = '|';
        }
        if(room->has_secret_door&&have_secrest==0){
            secret_room(map,i);
            have_secrest=1;
        }
        else if(room->has_password_door&&have_password==0){
            password_room(map,i);
            have_password=1;
        }
    }
}

void print_map(Map *map, int max_width, int max_height) {
    clear();
    for (int y = 0; y < max_height; y++) {
        for (int x = 0; x < max_width; x++) {
            mvprintw(y + 1, x, "%c", map->map[y][x]); // یک خط از بالا برای پیام‌ها خالی می‌ماند
        }
    }
    refresh();
}
