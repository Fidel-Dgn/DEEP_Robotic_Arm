/**
 *******************************************************************************
 * @file	stm32g4_epaper_paint.c
 * @author	vchav
 * @date	May 29, 2024
 *******************************************************************************
 * @brief	Paint tools
 * @author	Yehui from Waveshare
 *  
 * Copyright (C) Waveshare     July 28 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "config.h"
#if USE_EPAPER
#include "epaper/stm32g4_epaper_paint.h"


void Paint_Init(Paint* paint, unsigned char* image, int width, int height) {
    paint->rotate = ROTATE_0;
    paint->image = image;
    /* 1 byte = 8 pixels, la largeur de l'image doit donc être un multiple de 8 */
    paint->width = width % 8 ? width + 8 - (width % 8) : width;
    paint->height = height;
}

/**
 * @brief Efface l'image.
 * @param paint: Pointeur vers l'objet Paint à effacer.
 * @param colored: Couleur avec laquelle l'image doit être effacée.
 */
void Paint_Clear(Paint* paint, int colored) {
    for (int x = 0; x < paint->width; x++) {
        for (int y = 0; y < paint->height; y++) {
            Paint_DrawAbsolutePixel(paint, x, y, colored);
        }
    }
}

/**
 * @brief Dessine un pixel aux coordonnées absolues.
 * Cette fonction n'est pas affectée par le paramètre de rotation.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x: Coordonnée x absolue du pixel.
 * @param y: Coordonnée y absolue du pixel.
 * @param colored: Couleur du pixel.
 */
void Paint_DrawAbsolutePixel(Paint* paint, int x, int y, int colored) {
    if (x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
        return;
    }
    if (IF_INVERT_COLOR) {
        if (colored) {
            paint->image[(x + y * paint->width) / 8] |= 0x80 >> (x % 8);
        } else {
            paint->image[(x + y * paint->width) / 8] &= ~(0x80 >> (x % 8));
        }
    } else {
        if (colored) {
            paint->image[(x + y * paint->width) / 8] &= ~(0x80 >> (x % 8));
        } else {
            paint->image[(x + y * paint->width) / 8] |= 0x80 >> (x % 8);
        }
    }
}

/* Getters et Setters */

unsigned char* Paint_GetImage(Paint* paint) {
    return paint->image;
}

int Paint_GetWidth(Paint* paint) {
    return paint->width;
}

void Paint_SetWidth(Paint* paint, int width) {
    paint->width = width % 8 ? width + 8 - (width % 8) : width;
}

int Paint_GetHeight(Paint* paint) {
    return paint->height;
}

void Paint_SetHeight(Paint* paint, int height) {
    paint->height = height;
}

int Paint_GetRotate(Paint* paint) {
    return paint->rotate;
}

void Paint_SetRotate(Paint* paint, int rotate){
    paint->rotate = rotate;
}

/**
 * @brief Dessine un pixel aux coordonnées spécifiées.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x: Coordonnée x du pixel.
 * @param y: Coordonnée y du pixel.
 * @param colored: Couleur du pixel.
 */
void Paint_DrawPixel(Paint* paint, int x, int y, int colored) {
    int point_temp;
    if (paint->rotate == ROTATE_0) {
        if(x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
            return;
        }
        Paint_DrawAbsolutePixel(paint, x, y, colored);
    } else if (paint->rotate == ROTATE_90) {
        if(x < 0 || x >= paint->height || y < 0 || y >= paint->width) {
          return;
        }
        point_temp = x;
        x = paint->width - y;
        y = point_temp;
        Paint_DrawAbsolutePixel(paint, x, y, colored);
    } else if (paint->rotate == ROTATE_180) {
        if(x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
          return;
        }
        x = paint->width - x;
        y = paint->height - y;
        Paint_DrawAbsolutePixel(paint, x, y, colored);
    } else if (paint->rotate == ROTATE_270) {
        if(x < 0 || x >= paint->height || y < 0 || y >= paint->width) {
          return;
        }
        point_temp = x;
        x = y;
        y = paint->height - point_temp;
        Paint_DrawAbsolutePixel(paint, x, y, colored);
    }
}

/**
 * @brief Dessine un caractère sur le buffer de trame sans rafraîchissement.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x: Coordonnée x du point de départ du caractère.
 * @param y: Coordonnée y du point de départ du caractère.
 * @param ascii_char: Caractère ASCII à afficher.
 * @param font: Pointeur vers la structure de police de caractères.
 * @param colored: Couleur du caractère.
 */
void Paint_DrawCharAt(Paint* paint, int x, int y, char ascii_char, sFONT* font, int colored) {
    int i, j;
    unsigned int char_offset = (ascii_char - ' ') * font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
    const unsigned char* ptr = &font->table[char_offset];

    for (j = 0; j < font->Height; j++) {
        for (i = 0; i < font->Width; i++) {
            if (*ptr & (0x80 >> (i % 8))) {
                Paint_DrawPixel(paint, x + i, y + j, colored);
            }
            if (i % 8 == 7) {
                ptr++;
            }
        }
        if (font->Width % 8 != 0) {
            ptr++;
        }
    }
}

/**
 * @brief Affiche une chaîne de caractères sur le buffer de trame mais sans rafraîchissement.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x: Coordonnée x du point de départ de la chaîne de caractères.
 * @param y: Coordonnée y du point de départ de la chaîne de caractères.
 * @param text: Pointeur vers la chaîne de caractères à afficher.
 * @param font: Pointeur vers la structure de police de caractères utilisée.
 * @param colored: Couleur de la chaîne de caractères.
 */
void Paint_DrawStringAt(Paint* paint, int x, int y, const char* text, sFONT* font, int colored) {
    const char* p_text = text;
    unsigned int counter = 0;
    int refcolumn = x;
    
    /* Envoie la chaîne de caractères caractère par caractère sur l'EPD */
    while (*p_text != 0) {
        /* Affiche un caractère sur l'EPD */
        Paint_DrawCharAt(paint, refcolumn, y, *p_text, font, colored);
        /* Décrémente la position de la colonne de la largeur de la police */
        refcolumn += font->Width;
        /* Passe au caractère suivant */
        p_text++;
        counter++;
    }
}


/**
 * @brief Dessine une ligne.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x0: Coordonnée x du point de départ de la ligne.
 * @param y0: Coordonnée y du point de départ de la ligne.
 * @param x1: Coordonnée x du point d'arrivée de la ligne.
 * @param y1: Coordonnée y du point d'arrivée de la ligne.
 * @param colored: Couleur de la ligne.
 */
void Paint_DrawLine(Paint* paint, int x0, int y0, int x1, int y1, int colored) {
    /* Algorithme de Bresenham */
    int dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while ((x0 != x1) && (y0 != y1)) {
        Paint_DrawPixel(paint, x0, y0, colored);
        if (2 * err >= dy) {
            err += dy;
            x0 += sx;
        }
        if (2 * err <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}


/**
 * @brief Dessine une ligne horizontale.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x: Coordonnée x du point de départ de la ligne.
 * @param y: Coordonnée y du point de départ de la ligne.
 * @param line_width: Largeur de la ligne.
 * @param colored: Couleur de la ligne.
 */
void Paint_DrawHorizontalLine(Paint* paint, int x, int y, int line_width, int colored) {
    int i;
    for (i = x; i < x + line_width; i++) {
        Paint_DrawPixel(paint, i, y, colored);
    }
}

/**
 * @brief Dessine une ligne verticale.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x: Coordonnée x du point de départ de la ligne.
 * @param y: Coordonnée y du point de départ de la ligne.
 * @param line_height: Hauteur de la ligne.
 * @param colored: Couleur de la ligne.
 */
void Paint_DrawVerticalLine(Paint* paint, int x, int y, int line_height, int colored) {
    int i;
    for (i = y; i < y + line_height; i++) {
        Paint_DrawPixel(paint, x, i, colored);
    }
}


/**
 * @brief Dessine un rectangle.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x0: Coordonnée x du coin supérieur gauche du rectangle.
 * @param y0: Coordonnée y du coin supérieur gauche du rectangle.
 * @param x1: Coordonnée x du coin inférieur droit du rectangle.
 * @param y1: Coordonnée y du coin inférieur droit du rectangle.
 * @param colored: Couleur du rectangle.
 */
void Paint_DrawRectangle(Paint* paint, int x0, int y0, int x1, int y1, int colored) {
    int min_x, min_y, max_x, max_y;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;
    
    Paint_DrawHorizontalLine(paint, min_x, min_y, max_x - min_x + 1, colored);
    Paint_DrawHorizontalLine(paint, min_x, max_y, max_x - min_x + 1, colored);
    Paint_DrawVerticalLine(paint, min_x, min_y, max_y - min_y + 1, colored);
    Paint_DrawVerticalLine(paint, max_x, min_y, max_y - min_y + 1, colored);
}

/**
 * @brief Dessine un rectangle rempli.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x0: Coordonnée x du coin supérieur gauche du rectangle.
 * @param y0: Coordonnée y du coin supérieur gauche du rectangle.
 * @param x1: Coordonnée x du coin inférieur droit du rectangle.
 * @param y1: Coordonnée y du coin inférieur droit du rectangle.
 * @param colored: Couleur du rectangle.
 */
void Paint_DrawFilledRectangle(Paint* paint, int x0, int y0, int x1, int y1, int colored) {
    int min_x, min_y, max_x, max_y;
    int i;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;
    
    for (i = min_x; i <= max_x; i++) {
      Paint_DrawVerticalLine(paint, i, min_y, max_y - min_y + 1, colored);
    }
}


/**
 * @brief Dessine un cercle.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x: Coordonnée x du centre du cercle.
 * @param y: Coordonnée y du centre du cercle.
 * @param radius: Rayon du cercle.
 * @param colored: Couleur du cercle.
 */
void Paint_DrawCircle(Paint* paint, int x, int y, int radius, int colored) {
    /* Algorithme de Bresenham */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        Paint_DrawPixel(paint, x - x_pos, y + y_pos, colored);
        Paint_DrawPixel(paint, x + x_pos, y + y_pos, colored);
        Paint_DrawPixel(paint, x + x_pos, y - y_pos, colored);
        Paint_DrawPixel(paint, x - x_pos, y - y_pos, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if (-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}


/**
 * @brief Dessine un cercle rempli.
 * @param paint: Pointeur vers l'objet Paint utilisé pour le dessin.
 * @param x: Coordonnée x du centre du cercle.
 * @param y: Coordonnée y du centre du cercle.
 * @param radius: Rayon du cercle.
 * @param colored: Couleur du cercle
 */
void Paint_DrawFilledCircle(Paint* paint, int x, int y, int radius, int colored) {
    /* Algorithme de Bresenham */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        Paint_DrawPixel(paint, x - x_pos, y + y_pos, colored);
        Paint_DrawPixel(paint, x + x_pos, y + y_pos, colored);
        Paint_DrawPixel(paint, x + x_pos, y - y_pos, colored);
        Paint_DrawPixel(paint, x - x_pos, y - y_pos, colored);
        Paint_DrawHorizontalLine(paint, x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored);
        Paint_DrawHorizontalLine(paint, x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if(e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while(x_pos <= 0);
}

/* END OF FILE */

#endif





















