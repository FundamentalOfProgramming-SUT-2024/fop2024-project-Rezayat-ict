#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
//
// Function declarations
void create_user();
void generate_random_password(char *password, int length);
void print_menu(WINDOW *menu_win, int highlight, char **choices, int n_choices);
int start_menu();
int is_username_taken(const char *username);
int user_entrance_menu();
void reset_password_help();
int main() {
    initscr();
    clear();
    noecho();
    cbreak();  // Line buffering disabled. pass on everything
    curs_set(0);

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
                if (user_entrance_menu()) {
                    printw("Login successful\n");
                } else {
                    printw("Invalid username or password\n");
                }
                getch();
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
    fprintf(file_user, "Username: %s\nPassword: %s\nEmail: %s\n\n", username, password, email);
    fclose(file_user);

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
int user_entrance_menu(){
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
        return 0;
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
    } else {
        mvwprintw(input_win, 5, 1, "Invalid username or password");
    }
    wrefresh(input_win);
    getch();
    delwin(input_win);

    return authenticated;
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
        mvwprintw(input_win, 5, 1, "Invalid username or email. Please try again.");
    }
    wrefresh(input_win);
    getch();
    delwin(input_win);
}

  /*
void create_user() {
    clear();
    cbreak();
    noecho();          // Don't echo() while we do getch
    keypad(stdscr, TRUE);
    curs_set(1);       // Hide cursor
    char username[50];
    char password[50];
    char email[50];
    FILE *file;
    
    // Initialize ncurses window for input
    WINDOW *input_win = newwin(10, 50, (LINES - 10) / 2, (COLS - 50) / 2);
    box(input_win, 0, 0);
    refresh();
    wrefresh(input_win);
    keypad(input_win, TRUE);

    // Get username
    mvwprintw(input_win, 1, 1, "Enter Username: ");
    echo();  // Enable echo to see the input
    mvwgetstr(input_win, 1, 17, username);
    noecho();  // Disable echo again

    // Get password
    mvwprintw(input_win, 3, 1, "Enter Password: ");
    echo();
    mvwgetstr(input_win, 3, 17, password);
    noecho();

    // Get email
    mvwprintw(input_win, 5, 1, "Enter Email: ");
    echo();
    mvwgetstr(input_win, 5, 17, email);
    noecho();

    // Check constraints
    if (strlen(password) < 7) {
        mvwprintw(input_win, 7, 1, "Password must be at least 7 characters long");
        wrefresh(input_win);
        getch();
        create_user();
    }

    int has_digit = 0, has_upper = 0, has_lower = 0;
    for (int i = 0; i < strlen(password); i++) {
        if (isdigit(password[i])) has_digit = 1;
        if (isupper(password[i])) has_upper = 1;
        if (islower(password[i])) has_lower = 1;
    }
    if (!has_digit || !has_upper || !has_lower) {
        mvwprintw(input_win, 7, 1, "Password must contain at least one digit, one upper case letter, and one lower case letter");
        wrefresh(input_win);
        getch();
        create_user();
    }

    if (strstr(email, "@") == NULL || strstr(email, ".") == NULL) {
        mvwprintw(input_win, 7, 1, "Invalid email format");
        wrefresh(input_win);
        getch();
        create_user();
    }

    // Save user information to file
    file = fopen("users.txt", "a");
    if (file == NULL) {
        mvwprintw(input_win, 7, 1, "Error opening file");
        wrefresh(input_win);
        getch();
        return;
    }
    fprintf(file, "Username: %s\nPassword: %s\nEmail: %s\n\n", username, password, email);
    fclose(file);

    mvwprintw(input_win, 7, 1, "User created successfully!");
    wrefresh(input_win);
    getch();
}*/