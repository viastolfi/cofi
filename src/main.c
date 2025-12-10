#include <dirent.h>
#include <stdio.h>
#include <string.h>

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

int get_applications_from_dir(const char* path, files_t* fda)
{
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (path)) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      if (strcmp(ent->d_name, "..") == 0 ||
          strcmp(ent->d_name, ".") == 0)
        continue;
      da_append(fda, ent->d_name);
    }
    closedir (dir);
  } else {
    return 0;
  }

  return 1;
}

int main()
{
  files_t applications = {0};

  if (!get_applications_from_dir("/usr/share/applications", &applications)) {
    printf("error while reading applications file\n");
    return 1; 
  }

  /*
  da_foreach(char*, it, &applications) {
    printf("%s\n", (*it)); 
  }
  */

  InitWindow(400, 200, "raygui - controls test suite");
  SetTargetFPS(60);

  bool showMessageBox = false;

  while (!WindowShouldClose())
  {
    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

    if (GuiButton((Rectangle){ 24, 24, 120, 30 }, "#191#Show Message")) showMessageBox = true;

    if (showMessageBox)
    {
      int result = GuiMessageBox((Rectangle){ 85, 70, 250, 100 },
          "#191#Message Box", "Hi! This is a message!", "Nice;Cool");

      if (result >= 0) showMessageBox = false;
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
