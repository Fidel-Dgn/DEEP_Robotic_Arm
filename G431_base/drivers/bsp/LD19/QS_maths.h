/*
 *  Club Robot ESEO 2014
 *
 *  Fichier : QS_maths.h
 *  Package : Qualité Soft
 *  Description : Regroupement de toutes les fonctions mathématiques utiles
 *  Auteur : Arnaud
 *  Version 20130518
 */

/** ----------------  Defines possibles  --------------------
 *	FAST_COS_SIN				: Calcul rapide des cos et sin à l'aide d'un GRAND tableau de valeur
 */

#ifndef QS_MATHS_H
	#define QS_MATHS_H
	#include <stdbool.h>
	#include <stdint.h>
	#include "math.h"

	#define SQUARE(x)		((x)*(x))
	#define POWER3(x)		((x)*(x)*(x))
	#define RAD_TO_DEG(x)	((x)*180/PI4096)
	#define DEG_TO_RAD(x)	((x)*PI4096/180.0)


	// IMPORTANT IL FAUT UTILISER UN TABLEAU DE TYPE int32_t, sinon ça va merder violement !

	// L'utilisation d'un filtre à temps futur ou passé introduit un déphasage (avance pour temps futur, retard pour temps passé)
	// Pour contrer ce déphasage il est conseillé d'utiliser les deux filtres à la fois.
	// ex :
	//		filter_future_time(values, 30, (float[]){1/2., 1/2.}, 2);
	//		filter_past_time(values, 30, (float[]){1/2., 1/2.}, 2);
	//
	//	Subtilité pour dérivé votre tableau de donnée faite :
	//		filter_future_time(values, 30, (float[]){1, -1}, 2);

	/** Filtre à temps future
	 * @param values	: tableau de donnée à traiter
	 * @param nb_value	: nombre de valeur à traiter dans le tableau
	 * @param factor	: tableau des facteurs
	 * @param nb_factor	: nombre de facteurs
	 *
	 * @brief exemple : filtre moyenneur sur deux valeurs :
	 *						filter_future_time(values, 30, (float[]){1/2., 1/2.}, 2);
	 *						résultat pour la première valeur :	values[0] = 1/2 * values[0] + 1/2 values[1];
	 *						effet de bord : values[29] = values[29];
	 */
	void filter_future_time(int32_t values[], uint16_t nb_value, float factor[], uint8_t nb_factor);

	/** Filtre à temps passé
		 * @param values	: tableau de donnée à traiter
		 * @param nb_value	: nombre de valeur à traiter dans le tableau
		 * @param factor	: tableau des facteurs
		 * @param nb_factor	: nombre de facteurs
		 *
		 * @brief exemple : filtre moyenneur sur deux valeurs :
		 *						filter_past_time(values, 30, (float[]){1/2., 1/2.}, 2);
		 *						résultat pour la première valeur :	values[29] = 1/2 * values[29] + 1/2 values[28];
		 *						effet de bord : values[0] = values[0];
		 */
	void filter_past_time(int32_t values[], uint16_t nb_value, float factor[], uint8_t nb_factor);

	uint16_t searchMin(int32_t values[], uint16_t nbValue);
	uint16_t searchMax(int32_t values[], uint16_t nbValue);

	void COS_SIN_4096_get(int16_t teta,int16_t * cos, int16_t * sin);
	void COS_SIN_16384_get(int32_t teta, int16_t * cos, int16_t * sin);

	int16_t GEOMETRY_modulo_angle(int16_t angle);

	double cos4096(int16_t angle);
	double sin4096(int16_t angle);
	double tan4096(int angle);
	int16_t atan4096(double tangent);
	int16_t atan2_4096(double y, double x);

	uint64_t factorielle (uint32_t nb);

	int16_t rad2deg(int16_t angle);

	typedef struct {
		int16_t x;
		int16_t y;
	} GEOMETRY_point_t;

	typedef struct {
		int16_t x;
		int16_t y;
		int16_t a;
	} GEOMETRY_position_t;

	typedef struct {
		int16_t x;
		int16_t y;
	} GEOMETRY_vector_t;

	typedef struct {
		GEOMETRY_point_t a;
		GEOMETRY_point_t b;
	} GEOMETRY_segment_t;

	typedef struct {
		GEOMETRY_point_t c;
		uint16_t r;
	} GEOMETRY_circle_t;

	typedef struct {
		int16_t x1;
		int16_t x2;
		int16_t y1;
		int16_t y2;
	} GEOMETRY_rectangle_t;

	uint16_t GEOMETRY_distance(GEOMETRY_point_t a, GEOMETRY_point_t b);
	uint32_t GEOMETRY_distance_square(GEOMETRY_point_t a, GEOMETRY_point_t b);
	uint32_t GEOMETRY_pythagore(uint32_t d1, uint32_t d2);
	uint16_t GEOMETRY_manhattan_distance(GEOMETRY_point_t a, GEOMETRY_point_t b);
	bool GEOMETRY_segments_intersects(GEOMETRY_segment_t s1,GEOMETRY_segment_t s2);
	bool GEOMETRY_segments_parallel(GEOMETRY_segment_t seg1, GEOMETRY_segment_t seg2);
	int16_t GEOMETRY_viewing_angle(int16_t start_x, int16_t start_y, int16_t destination_x, int16_t destination_y);
	int32_t GEOMETRY_viewing_algebric_distance(int32_t start_x, int32_t start_y, int32_t destination_x, int32_t destination_y, int32_t angle_de_vue);
	int32_t GEOMETRY_viewing_algebric_distance_mm16(int32_t start_x, int32_t start_y, int32_t destination_x, int32_t destination_y, int32_t angle_de_vue);

	int32_t GEOMETRY_modulo_angle_22(int32_t angle);
	GEOMETRY_point_t GEOMETRY_segment_middle(GEOMETRY_segment_t s);
	GEOMETRY_circle_t GEOMETRY_circle_from_diameter(GEOMETRY_segment_t diameter);
	GEOMETRY_point_t GEOMETRY_circle_from_3_points(GEOMETRY_point_t A, GEOMETRY_point_t B, GEOMETRY_point_t C);
	GEOMETRY_segment_t GEOMETRY_circle_intersections(GEOMETRY_circle_t c0, GEOMETRY_circle_t c1);

	/**
	 * @brief GEOMETRY_proj_on_line
	 * @param seg Le segment définissat la droite sur laquelle faire le projeté orthogonal
	 * @param pointToProj Le point à projeté
	 * @return Le projeté orthogonale
	 */
	GEOMETRY_point_t GEOMETRY_proj_on_line(GEOMETRY_segment_t seg, GEOMETRY_point_t pointToProj);
	void GEOMETRY_where_to_go_to_take(int32_t * x_consigne, int32_t * y_consigne, int32_t buoy_x, int32_t buoy_y, int32_t x_emplacement, int32_t y_emplacement, int32_t push_buoy_over, int32_t * x_act_event, int32_t * y_act_event, int32_t dist_act_event);

	bool is_in_square(int16_t x1, int16_t x2, int16_t y1, int16_t y2, GEOMETRY_point_t current);
	bool is_in_rectangle(GEOMETRY_rectangle_t rect, GEOMETRY_point_t current);
	bool is_in_circle(GEOMETRY_point_t current, GEOMETRY_circle_t circle);

	/**
	 * @brief is_in_quadri
	 * @pre Les points doivent être donné pour que le quadrilatère soit connexe (chaques points voit tout les points)
	 * @param points	Le tableau de point du quadrilatère (doit être dans l'ordre)
	 * @param tested_point Le point à tester
	 * @return si le point est dans le quadrilatère
	 */
	bool is_in_quadri(GEOMETRY_point_t points[4], GEOMETRY_point_t tested_point);

	/**
	 * @brief is_in_polygon
	 * @param polygon Les points du polygone
	 * @param nb_summits Le nombre de sommets
	 * @param tested_point Le point que l'on veut tester si il est dans le polygone
	 * @param out_point Un point dont on est sur qu'il est en dehors du polygone (par exemple en dehors du terrain
	 * @return
	 */
	bool is_in_polygon(GEOMETRY_point_t polygon[], uint8_t nb_summits, GEOMETRY_point_t tested_point, GEOMETRY_point_t out_point, uint8_t *stock_intersections);

	double GEOMETRY_atof(char *s);
	#define PI4096				12868
	#define PI16384				51472
	#define HALF_PI16384		25736
	#define QUATER_PI16384		12868
	#define THREE_HALF_PI16384	77208
	#define	TWO_PI16384			102944
	#define PI_22				(13176795)		/*Valeur de PI<<22 */
	#define TWO_PI22			(26353589)
	#define PI_28				(843314856)		/*Valeur de PI<<28 */
	#define TWO_PI28			(1686629713)

	#define TWO_PI4096			(25736)
	#define HALF_PI4096			(6434)
	#define THREE_HALF_PI4096	(19302)

#endif /* ndef QS_MATHS_H */
