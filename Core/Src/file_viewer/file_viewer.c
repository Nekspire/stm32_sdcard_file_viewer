#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "file_viewer.h"

char message[30];
unsigned int selector_position = SELECTOR_POS_1;
Point selector_pixel_position;
unsigned int item = 0;
long unsigned int total_items = 0;
char items_fname[ITEMS][MAX_FILENAME_CHAR];
unsigned int slash = 1;
unsigned int item_pixel_x = 0;

static size_t strlprecat( char* dst, const char * src, size_t size) {
  size_t dstlen = strnlen( dst, size);
  size_t srclen = strlen( src);
  size_t srcChars = srclen;
  size_t dstChars = dstlen;

  if (0 == size) {
    /* we can't write anything into the dst buffer */
    /*  -- bail out                                */

    return( srclen + dstlen);
  }

  /* determine how much space we need to add to front of dst */

  if (srcChars >= size) {
    /* we can't even fit all of src into dst */
    srcChars = size - 1;
  }

  /* now figure out how many of dst's characters will fit in the remaining buffer */
  if ((srcChars + dstChars) >= size) {
    dstChars = size - srcChars - 1;
  }

  /* move dst to new location, truncating if necessary */
  memmove( dst+srcChars, dst, dstChars);

  /* copy src into the spot we just made for it */
  memcpy( dst, src, srcChars);

  /* make sure the whole thing is terminated properly */
  dst[srcChars + dstChars] = '\0';

  return( srclen + dstlen);
}

static void display_items_fname_by_char(FileViewer *viewer, unsigned int index) {
  int str_len = (int) strlen(items_fname[index]);

  for (int j = 0; j < MAX_ITEM_LINECHAR; j++) {
    if (j < str_len) {
      BSP_LCD_DisplayChar(item_pixel_x + (j * viewer->items_display_properties.pFont->Width),
                          (SELECTOR_POS_1 + index) * viewer->items_display_properties.pFont->Height,
                          items_fname[index][j]);
    } else {
      BSP_LCD_DisplayChar(item_pixel_x + (j * viewer->items_display_properties.pFont->Width),
                          (SELECTOR_POS_1 + index) * viewer->items_display_properties.pFont->Height,
                          ' ');
    }
  }
}

void FileViewer_unwrap_item_name(FileViewer *viewer) {
  if (strlen(items_fname[selector_position - SELECTOR_POS_1]) > MAX_ITEM_LINECHAR) {
    // temporary buffer 1
    char temp[MAX_FILENAME_CHAR];
    // copy items_fname[selector_position - SELECTOR_POS_1] to temporary buffer
    memcpy(temp, items_fname[selector_position - SELECTOR_POS_1],
           sizeof(items_fname[selector_position - SELECTOR_POS_1]));

    unsigned int strlen_temp = strlen(temp);
    int delta = (int) strlen_temp - MAX_ITEM_LINECHAR;
    // shift left 1 character
    for (int i = 0; i < delta; i++) {
      for (int j = 0; j < strlen_temp; j++) {
        temp[j] = temp[j + 1];
      }
      for (int j = 0; j < MAX_ITEM_LINECHAR; j++) {
        BSP_LCD_DisplayChar(item_pixel_x + (j * viewer->items_display_properties.pFont->Width),
                            selector_position * viewer->items_display_properties.pFont->Height,
                            temp[j]);
      }
      DELAY_MS(15);
    }
    DELAY_MS(500);
    // directly back to initial position
    for (int j = 0; j < MAX_ITEM_LINECHAR; j++) {
      BSP_LCD_DisplayChar(item_pixel_x + (j * viewer->items_display_properties.pFont->Width),
                          selector_position * viewer->items_display_properties.pFont->Height,
                          items_fname[selector_position - SELECTOR_POS_1][j]);
    }

    // shift right 1 character
    // scroll back to initial position
    /* for (int i = 0; i < delta; i++) {
      for (int j = MAX_ITEM_LINECHAR - 1; j > 0; j--) {
        temp[j] = temp[j - 1];
      }
      temp[0] = items_fname[selector_position - SELECTOR_POS_1][delta - i - 1];
      if()
      BSP_LCD_SetTextColor(viewer->display_properties.TextColor);
      BSP_LCD_DisplayStringAt(item_pixel_x, selector_position * viewer->display_properties.pFont->Height,
                              (uint8_t *) temp, LEFT_MODE);
    } */
  }
}

bool FileViewer_init(FileViewer *viewer) {
  FRESULT fr_mount, fr_result;
  uint8_t lcd_result;

  lcd_result = BSP_LCD_Init();

  if (lcd_result == LCD_OK) {
    item_pixel_x = viewer->items_display_properties.pFont->Width + 1;

    BSP_LCD_Clear(viewer->items_display_properties.BackColor);
    // path display properties
    BSP_LCD_SetFont(viewer->path_display_properties.pFont);
    BSP_LCD_SetTextColor(viewer->path_display_properties.TextColor);

    fr_mount = f_mount(viewer->fs, viewer->path, 0);

    if (fr_mount == FR_OK) {
      fr_result = f_getcwd(viewer->path, sizeof(viewer->path));

      if (fr_result == FR_OK) {
        BSP_LCD_DisplayStringAt(0, 0, (uint8_t *) viewer->path, LEFT_MODE);

        fr_result = f_findfirst(viewer->dir, viewer->filinfo, viewer->path, "*");

        // items display properties
        BSP_LCD_SetFont(viewer->items_display_properties.pFont);
        BSP_LCD_SetTextColor(viewer->items_display_properties.TextColor);
        if (fr_result == FR_OK && viewer->filinfo->fname[0]) {
          BSP_LCD_DisplayStringAt(SELECTOR_X, 2 * viewer->items_display_properties.pFont->Height,
                                  (uint8_t *) SELECTOR_TYPE,
                                  LEFT_MODE);
          selector_pixel_position.X = (int16_t) SELECTOR_X;
          selector_pixel_position.Y = (int16_t) (2 * viewer->items_display_properties.pFont->Height);
        }

        while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
          if (item < ITEMS) {
            memcpy(items_fname[item], viewer->filinfo->fname, sizeof(viewer->filinfo->fname));
            display_items_fname_by_char(viewer, item);
            item += 1;
            fr_result = f_findnext(viewer->dir, viewer->filinfo);
          } else {
            break;
          }
        }
        total_items = item;

      } else {
        sprintf(message, "%s open failed", DRIVE);
        BSP_LCD_DisplayStringAt(0, 0, (uint8_t *) message, LEFT_MODE);
        return false;
      }

      return true;
    } else {
      sprintf(message, "%s open failed", DRIVE);
      BSP_LCD_DisplayStringAt(0, 0, (uint8_t *) message, LEFT_MODE);
      return false;
    }
  } else
    return false;
}


void FileViewer_enter_directory(FileViewer *viewer) {
  FRESULT fr_result;

  if (strchr(items_fname[selector_position - SELECTOR_POS_1], '.') == NULL) {
    // clear old path on display
    BSP_LCD_SetTextColor(viewer->path_display_properties.BackColor);
    BSP_LCD_SetFont(viewer->path_display_properties.pFont);
    BSP_LCD_DisplayStringAt(0, 0, (uint8_t *) viewer->path,
                            LEFT_MODE);
    // add to path new directory
    if (viewer->path[strlen(viewer->path) - 1] != '/') {
      strcat(viewer->path, "/");
      slash += 1;
    }
    strcat(viewer->path, items_fname[selector_position - SELECTOR_POS_1]);
    // display new path
    BSP_LCD_SetTextColor(viewer->path_display_properties.TextColor);
    BSP_LCD_DisplayStringAt(0, 0, (uint8_t *) viewer->path,
                            LEFT_MODE);
    // clear last item selector
    BSP_LCD_SetFont(viewer->items_display_properties.pFont);
    BSP_LCD_SetTextColor(viewer->items_display_properties.BackColor);
    BSP_LCD_DisplayStringAt(SELECTOR_X, selector_position * viewer->items_display_properties.pFont->Height,
                            (uint8_t *) SELECTOR_TYPE,
                            LEFT_MODE);
    // close dir
    f_closedir(viewer->dir);
    // find first item in path
    fr_result = f_findfirst(viewer->dir, viewer->filinfo, viewer->path, "*");
    // display item selector if there are items in directory
    if (fr_result == FR_OK && viewer->filinfo->fname[0]) {
      // set initial selector position
      selector_position = SELECTOR_POS_1;
      BSP_LCD_SetFont(viewer->items_display_properties.pFont);
      // display selector on screen
      BSP_LCD_SetTextColor(viewer->items_display_properties.TextColor);
      BSP_LCD_DisplayStringAt(SELECTOR_X, SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height,
                              (uint8_t *) SELECTOR_TYPE,
                              LEFT_MODE);
      selector_pixel_position.X = (int16_t) SELECTOR_X;
      selector_pixel_position.Y = (int16_t) (SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height);
    }
    // reset items name buffer
    memset(items_fname, 0, sizeof items_fname);
    // reset item counter
    item = 0;
    while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
      if (item < ITEMS) {
        memcpy(items_fname[item], viewer->filinfo->fname, sizeof(viewer->filinfo->fname));
        display_items_fname_by_char(viewer, item);
        item += 1;
        fr_result = f_findnext(viewer->dir, viewer->filinfo);
      } else {
        break;
      }
    }
    if (item != ITEMS) {
      // clear additional items from last page
      for (int i = item; i < ITEMS; i++) {
        display_items_fname_by_char(viewer, i);
      }
    }
    total_items = item;
  }
}

void FileViewer_leave_directory(FileViewer *viewer) {
  FRESULT fr_result;

  int path_len = (int) strlen(viewer->path);
  // path is not root == "/"
  if (path_len > 1) {
    int chars = 0, i = path_len;
    // clear old path on display
    BSP_LCD_SetTextColor(viewer->path_display_properties.BackColor);
    BSP_LCD_SetFont(viewer->path_display_properties.pFont);
    BSP_LCD_DisplayStringAt(0, 0, (uint8_t *) viewer->path,
                            LEFT_MODE);
    // check '/' separator in path
    while (viewer->path[i] != '/') {
      i--;
      chars++;
    }
    if (slash >= 2) {
      path_len = path_len - chars;
      slash -= 1;
    } else {
      path_len = path_len - (chars - 1);
    }
    // trim path
    strlprecat(viewer->path, viewer->path, path_len + 1);
    // display new path
    BSP_LCD_SetTextColor(viewer->path_display_properties.TextColor);
    BSP_LCD_DisplayStringAt(0, 0, (uint8_t *) viewer->path,
                            LEFT_MODE);
    // clear last item selector
    BSP_LCD_SetFont(viewer->items_display_properties.pFont);
    BSP_LCD_SetTextColor(viewer->items_display_properties.BackColor);
    BSP_LCD_DisplayStringAt(SELECTOR_X, selector_position * viewer->items_display_properties.pFont->Height,
                            (uint8_t *) SELECTOR_TYPE,
                            LEFT_MODE);
    // set initial selector position
    selector_position = SELECTOR_POS_1;
    // find first item in path
    f_closedir(viewer->dir);
    fr_result = f_findfirst(viewer->dir, viewer->filinfo, viewer->path, "*");
    // display item selector if there are items in directory
    if (fr_result == FR_OK && viewer->filinfo->fname[0]) {
      // set initial selector position
      selector_position = SELECTOR_POS_1;
      // display selector on screen
      BSP_LCD_SetFont(viewer->items_display_properties.pFont);
      BSP_LCD_SetTextColor(viewer->items_display_properties.TextColor);
      BSP_LCD_DisplayStringAt(SELECTOR_X, SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height,
                              (uint8_t *) SELECTOR_TYPE,
                              LEFT_MODE);
      selector_pixel_position.X = (int16_t) SELECTOR_X;
      selector_pixel_position.Y = (int16_t) (SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height);
    }
    // reset items name buffer
    memset(items_fname, 0, sizeof items_fname);
    // reset item counter
    item = 0;
    while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
      if (item < ITEMS) {
        memcpy(items_fname[item], viewer->filinfo->fname, sizeof(viewer->filinfo->fname));
        display_items_fname_by_char(viewer, item);
        item += 1;
        fr_result = f_findnext(viewer->dir, viewer->filinfo);
      } else {
        break;
      }
    }
    if (item != ITEMS) {
      // clear additional items from last page
      for (int i = item; i < ITEMS; i++) {
        display_items_fname_by_char(viewer, i);
      }
    }
    total_items = item;
  }
}

void FileViewer_scroll_down(FileViewer *viewer) {
  FRESULT fr_result;

  if (selector_pixel_position.Y < (SELECTOR_POS_1 + item - 1) * viewer->items_display_properties.pFont->Height) {
    BSP_LCD_SetTextColor(viewer->items_display_properties.BackColor);
    BSP_LCD_DisplayStringAt(SELECTOR_X, selector_pixel_position.Y, (uint8_t *) SELECTOR_TYPE,
                            LEFT_MODE);

    selector_pixel_position.Y += (int16_t) viewer->items_display_properties.pFont->Height;
    selector_position += 1;

    BSP_LCD_SetTextColor(viewer->items_display_properties.TextColor);
    BSP_LCD_DisplayStringAt(SELECTOR_X, selector_pixel_position.Y, (uint8_t *) SELECTOR_TYPE,
                            LEFT_MODE);
  } else {
    // scrolling down
    if (total_items >= ITEMS) {
      unsigned long int item_pos = 0;
      // find first item in path
      f_closedir(viewer->dir);
      // start searching for item from beginning (inefficient way)
      fr_result = f_findfirst(viewer->dir, viewer->filinfo, viewer->path, "*");
      if (fr_result == FR_OK && viewer->filinfo->fname[0]) {
        item_pos += 1;
        while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
          if (item_pos == total_items + 1) {
            // increase total_times
            total_items += 1;

            for (int i = 0; i <= ITEMS - 1; i++) {
              // shift down items_fname[]
              if (i == ITEMS - 1) {
                memcpy(items_fname[i], viewer->filinfo->fname, strlen(viewer->filinfo->fname) + 1);
              } else {
                memcpy(items_fname[i], items_fname[i + 1], strlen(items_fname[i + 1]) + 1);
              }

              display_items_fname_by_char(viewer, i);
            }
            break;
          }
          fr_result = f_findnext(viewer->dir, viewer->filinfo);
          item_pos += 1;
        }
      }
    }
  }
}

void FileViewer_scroll_up(FileViewer *viewer) {
  FRESULT fr_result;

  if (selector_pixel_position.Y > SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height) {
    BSP_LCD_SetTextColor(viewer->items_display_properties.BackColor);
    BSP_LCD_DisplayStringAt(SELECTOR_X, selector_pixel_position.Y, (uint8_t *) SELECTOR_TYPE,
                            LEFT_MODE);

    selector_pixel_position.Y -= (int16_t) viewer->items_display_properties.pFont->Height;
    selector_position -= 1;

    BSP_LCD_SetTextColor(viewer->items_display_properties.TextColor);
    BSP_LCD_DisplayStringAt(SELECTOR_X, selector_pixel_position.Y, (uint8_t *) SELECTOR_TYPE,
                            LEFT_MODE);
  } else {
    unsigned long int item_pos = 0;
    // find first item in path
    f_closedir(viewer->dir);
    // start searching for item from beginning (inefficient way)
    fr_result = f_findfirst(viewer->dir, viewer->filinfo, viewer->path, "*");
    if (fr_result == FR_OK && viewer->filinfo->fname[0]) {
      item_pos += 1;
      while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
        if (item_pos == total_items - ITEMS) {
          // decrease total items
          total_items -= 1;

          for (int i = ITEMS - 1; i >= 0; i--) {
            // shift down items_fname[]
            if (i == 0) {
              memcpy(items_fname[i], viewer->filinfo->fname, strlen(viewer->filinfo->fname) + 1);
            } else {
              memcpy(items_fname[i], items_fname[i - 1], strlen(items_fname[i - 1]) + 1);
            }

            display_items_fname_by_char(viewer, i);
          }
          break;
        }
        fr_result = f_findnext(viewer->dir, viewer->filinfo);
        item_pos += 1;
      }
    }
  }
}

void FileViewer_scroll_page_right(FileViewer *viewer) {
  FRESULT fr_result;

  // scroll page to the next page
  if (total_items >= ITEMS) {
    unsigned long int item_pos = 0;
    // close dir
    f_closedir(viewer->dir);
    // find first item in path
    fr_result = f_findfirst(viewer->dir, viewer->filinfo, viewer->path, "*");
    if (fr_result == FR_OK && viewer->filinfo->fname[0]) {
      item_pos += 1;
      while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
        if (item_pos == total_items + 1) {
          // clear last item selector
          BSP_LCD_SetTextColor(viewer->items_display_properties.BackColor);
          BSP_LCD_DisplayStringAt(SELECTOR_X, selector_position * viewer->items_display_properties.pFont->Height,
                                  (uint8_t *) SELECTOR_TYPE,
                                  LEFT_MODE);
          // set initial selector position
          selector_position = SELECTOR_POS_1;
          // display selector on screen
          BSP_LCD_SetTextColor(viewer->items_display_properties.TextColor);
          BSP_LCD_DisplayStringAt(SELECTOR_X, SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height,
                                  (uint8_t *) SELECTOR_TYPE,
                                  LEFT_MODE);
          selector_pixel_position.X = (int16_t) SELECTOR_X;
          selector_pixel_position.Y = (int16_t) (SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height);
          // reset items name buffer
          memset(items_fname, 0, sizeof items_fname);
          // reset item counter
          item = 0;
          while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
            if (item < ITEMS) {
              memcpy(items_fname[item], viewer->filinfo->fname, sizeof(viewer->filinfo->fname));
              display_items_fname_by_char(viewer, item);
              item += 1;
              fr_result = f_findnext(viewer->dir, viewer->filinfo);
            } else {
              break;
            }
          }
          if (item != ITEMS) {
            // clear additional items from last page
            for (int i = item; i < ITEMS; i++) {
              display_items_fname_by_char(viewer, i);
            }
          }
          total_items += item;
          break;
        }
        fr_result = f_findnext(viewer->dir, viewer->filinfo);
        item_pos += 1;
      }
    }
  }
}

void FileViewer_scroll_page_left(FileViewer *viewer) {
  FRESULT fr_result;

  unsigned long int item_pos = 0;
  // close dir
  f_closedir(viewer->dir);
  // find first item in path
  fr_result = f_findfirst(viewer->dir, viewer->filinfo, viewer->path, "*");
  if (fr_result == FR_OK && viewer->filinfo->fname[0]) {
    item_pos += 1;
    while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
      if (item_pos == total_items - (2 * ITEMS) + 1) {
        // clear last item selector
        BSP_LCD_SetTextColor(viewer->items_display_properties.BackColor);
        BSP_LCD_DisplayStringAt(SELECTOR_X, selector_position * viewer->items_display_properties.pFont->Height,
                                (uint8_t *) SELECTOR_TYPE,
                                LEFT_MODE);
        // set initial selector position
        selector_position = SELECTOR_POS_1;
        // display selector on screen
        BSP_LCD_SetTextColor(viewer->items_display_properties.TextColor);
        BSP_LCD_DisplayStringAt(SELECTOR_X, SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height,
                                (uint8_t *) SELECTOR_TYPE,
                                LEFT_MODE);
        selector_pixel_position.X = (int16_t) SELECTOR_X;
        selector_pixel_position.Y = (int16_t) (SELECTOR_POS_1 * viewer->items_display_properties.pFont->Height);
        // reset items name buffer
        memset(items_fname, 0, sizeof items_fname);
        // reset item counter
        item = 0;
        while (fr_result == FR_OK && viewer->filinfo->fname[0]) {
          if (item < ITEMS) {
            memcpy(items_fname[item], viewer->filinfo->fname, sizeof(viewer->filinfo->fname));
            display_items_fname_by_char(viewer, item);
            item += 1;
            fr_result = f_findnext(viewer->dir, viewer->filinfo);
          } else {
            break;
          }
        }
        if (item != ITEMS) {
          // clear additional items from last page
          for (int i = item; i < ITEMS; i++) {
            display_items_fname_by_char(viewer, i);
          }
        }
        total_items -= item;
        break;
      }
      fr_result = f_findnext(viewer->dir, viewer->filinfo);
      item_pos += 1;
    }
  }
}