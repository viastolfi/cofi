#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "thirdparty/raygui.h"

#define DA_LIB_IMPLEMENTATION
#include "thirdparty/da.h"

typedef struct {
  char** items;
  size_t count;
  size_t capacity;
} files_t;

typedef struct {
  char* name;
  char* description;
  char* icons;
  char* exec;
} cofi_element_t;

typedef struct {
  cofi_element_t** items;
  size_t count;
  size_t capacity;
} cofis_t;

int exec_app(char* name)
{
  pid_t pid = fork();

  if (pid == 0) {
    char* cmd = strdup(name);
    char* arg = strchr(cmd, '%'); 
    if (arg != NULL) {
      arg--;
      *arg = '\0';  
    }

    char* args[] = {cmd, NULL};

    execvp(args[0], args);
    perror("execvp failed");
    exit(1);
  } else {
    return 1; 
  }
  return 0;
}

int get_applications_from_dir(const char* path, files_t* fda)
{
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(path)) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (strcmp(ent->d_name, "..") == 0 ||
          strcmp(ent->d_name, ".") == 0)
        continue;
      
      const char* dot = strchr(ent->d_name, '.');
      if (!dot || strcmp(dot, ent->d_name) == 0)
        continue;

      if (strcmp(dot, ".desktop") != 0)
        continue;

      da_append(fda, strdup(ent->d_name));
    }
    closedir(dir);
  } else {
    return 0;
  }

  return 1;
}

int get_metadata_from_apps(files_t* fda, cofis_t* cda)
{
  for (size_t i = 0; i < fda->count; ++i) {
    char* path = (char*) malloc(2048);
    if (!path) {
      printf("error while allocating memory\n");
      return 1;
    }
    strcpy(path, "/usr/share/applications/");
    strcat(path, fda->items[i]);

    FILE* f = fopen(path, "rb");
    if (!f) {
      printf("error while opening '.desktop' file\n"); 
      free(path);
      return 1;
    }
    free(path);

    char* text = (char*) malloc(1 << 20);      
    int len = f ? (int) fread(text, 1, 1 << 20, f) : -1;
    if (len < 0) {
      printf("error while reading content of '.desktop' file\n");    
      free(text);
      fclose(f);
      return 1;
    }
    fclose(f);

    cofi_element_t* el = (cofi_element_t*) calloc(1, sizeof(cofi_element_t));
    char* cursor = text;    
    char buffer[2048];
    int buffer_index = 0;

    cursor = strchr(cursor, '\n');
    cursor++;

    while (cursor < text + len) {
      if (*cursor != '=') {
        if (*cursor != '\n')
          buffer[buffer_index++] = *cursor; 
        cursor++;
        continue;
      } 
      buffer[buffer_index] = '\0';

      cursor++;
      char value_buffer[2048];
      int value_buffer_index = 0;
      while (*cursor != '\n' && cursor != text + len) 
        value_buffer[value_buffer_index++] = *cursor++;
      value_buffer[value_buffer_index] = '\0';

      if (strcmp(buffer, "Name") == 0 && el->name == NULL) {
        el->name = (char*) malloc(value_buffer_index + 1);
        strcpy(el->name, value_buffer);
      }
      
      if (strcmp(buffer, "Exec") == 0 && el->exec == NULL) {
        el->exec = (char*) malloc(value_buffer_index + 1); 
        strcpy(el->exec, value_buffer);
      }

      if (strcmp(buffer, "GenericName") == 0 && 
          el->description == NULL) {
        el->description = strdup(value_buffer);
      }

      cursor++;
      buffer_index = 0;
      value_buffer_index = 0;
    }

    da_append(cda, el);
    free(text);
  } 
}

char search[1024] = {0};

int compare(const void *a, const void *b) {
  const cofi_element_t* s1 = *(const cofi_element_t**)a;
  const cofi_element_t* s2 = *(const cofi_element_t**)b;

  if (search[0] == '\0') return strcmp(s1->name, s2->name);

  char *p1 = strcasestr(s1->name, search);
  char *p2 = strcasestr(s2->name, search);

  if (p1 && p2) {
    return (p1 - s1->name) - (p2 - s2->name);
  }
  if (p1) return -1; 
  if (p2) return 1; 

  return strcmp(s1->name, s2->name); 
}

void build_lv_text(cofis_t* datas, char* list)
{
  for (size_t i = 0; i < datas->count; ++i) {
    strcat(list, datas->items[i]->name); 
    if (datas->items[i]->description) {
      strcat(list, " (");
      strcat(list, datas->items[i]->description);
      strcat(list, ")");
    }
    strcat(list, ";");
  }
}

int main()
{
  files_t applications = {0};

  if (!get_applications_from_dir("/usr/share/applications", &applications)) {
    printf("error while reading applications file\n");
    return 1; 
  }

#if 0
  for (size_t i = 0; i < applications.count; ++i) {
    printf("%ld : %s\n", i, applications.items[i]); 
  }
#endif

  cofis_t datas = {0};
  get_metadata_from_apps(&applications, &datas);

#if 0
  for (size_t i = 0; i < datas.count; ++i) {
    printf("%ld : %s - %s\n", i, datas.items[i]->name, datas.items[i]->exec); 
  }
#endif

  SetConfigFlags(FLAG_WINDOW_UNDECORATED);
  InitWindow(400, 200, "raygui - controls test suite");
  SetTargetFPS(60);

  bool showMessageBox = false;

  Rectangle topleft = {1, 1, 39, 29};
  bool searched = false;

  char* list = malloc(1 << 20);
  strcpy(list, "");
  build_lv_text(&datas, list);

  int scroll = 0;
  int active = 0;

  GuiSetStyle(LISTVIEW, SCROLLBAR_WIDTH, 10);
  GuiSetStyle(LISTVIEW, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

  bool close_window = false;

  while (!WindowShouldClose() && !close_window)
  {
    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

    DrawRectangleRec(topleft, LIGHTGRAY);
    GuiLabel((Rectangle){10, 5, 30, 20}, "App");
    searched = GuiTextBox((Rectangle){41, 1, 358, 29}, search, 1024, true);
    GuiListView((Rectangle){1, 31, 398, 168}, list, &scroll, &active);

    if (IsKeyPressed(264) && active < datas.count) active++; scroll++;
    if (IsKeyPressed(265) && active > 0) active--; scroll--;


    qsort(datas.items, datas.count, sizeof(cofi_element_t*), compare);

    strncpy(list, "", sizeof(list));
    build_lv_text(&datas, list);

    if (searched) 
      if (exec_app(datas.items[active]->exec))
        close_window = true;

    EndDrawing();
  }

  CloseWindow();

  free(list);

  for (size_t i = 0; i < applications.count; ++i)
    free(applications.items[i]);
  free(applications.items);

  for (size_t i = 0; i < datas.count; ++i) {
    free(datas.items[i]->name); 
    free(datas.items[i]->exec); 
    if (datas.items[i]->description) 
      free(datas.items[i]->description);
    free(datas.items[i]);
  }
  free(datas.items);

  return 0;
}
