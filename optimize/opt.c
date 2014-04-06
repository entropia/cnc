#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <assert.h>

struct point {
	double x, y;
	struct point *next, *prev;
};

struct path {
	struct point *points;
	struct path *next, *prev;
};

void read_coords(const char *s, double *x, double *y, double *z) {
	for(; *s; s++) {
		double *p;

		switch(*s) {
			case 'X':
				p = x;
				break;
			case 'Y':
				p = y;
				break;
			case 'Z':
				p = z;
				break;
			default:
				continue;
		}

		s++;
		*p = atof(s);
	}
}

struct path *add_path(struct path **paths, struct point *points) {
	struct path *new = malloc(sizeof(struct path));
	memset(new, 0, sizeof(struct path));

	new->prev = NULL;
	new->next = *paths;
	if(*paths)
		(*paths)->prev = new;

	new->points = points;
	*paths = new;

	return new;
}

void remove_path(struct path **paths, struct path *path) {
	if(path == *paths)
		*paths = path->next;

	if(path->prev)
		path->prev->next = path->next;

	if(path->next)
		path->next->prev = path->prev;

	struct point *p = path->points;
	while(p) {
		struct point *next = p->next;

		free(p);
		p = next;
	}
}

struct path *get_paths(FILE *in) {
	char buf[512];

	double x, y, z;
	x = y = 0.0;
	z = DBL_MAX;

	struct path *all_paths = NULL;
	struct point *current_path = NULL;
	struct point *current_tail = NULL;

	while(fgets(buf, 512, in)) {
		if(*buf == '(')
			continue;

		if(*buf == 'M')
			continue;

		if(*buf != 'G') {
			fprintf(stderr, "unknown command in line: %s", buf);
			return NULL;
		}

		if(*buf == 'G' && memcmp(buf, "G00", 3) && memcmp(buf, "G01", 3))
			continue;

		read_coords(buf, &x, &y, &z);

		if(z > 0.0 && current_path) { // we moved up, end this path
			add_path(&all_paths, current_path);
			current_path = NULL;
			current_tail = NULL;

			continue;
		}

		if(z > 0.0) // we do not care about intermittent moves
			continue;

		if(current_tail && x == current_tail->x && y == current_tail->y)
			continue;

		struct point *new = malloc(sizeof(struct point));
		memset(new, 0, sizeof(struct point));

		new->prev = current_tail;
		new->next = NULL;
		if(current_tail)
			current_tail->next = new;

		if(!current_path)
			current_path = new;

		new->x = x;
		new->y = y;

		current_tail = new;
	}

	return all_paths;
}

#define ZSAFE 2.54

const char *preamble = "\
G21\n\
G90\n\
M3\n\
M7\n";

const char *postamble = "\
M5\n\
M9\n\
M2\n";

struct path *find_near_path(struct path **paths, double x, double y) {
	if(!*paths)
		return NULL;

	struct path *best_path = *paths;
	struct point *best_point = best_path->points;
	double best_dist = DBL_MAX;

	bool found = false;
	for(struct path *path = *paths; path; path = path->next) {
		for(struct point *point = path->points; point; point = point->next) {
			double dist = sqrt(pow(x - point->x, 2.0) + pow(y - point->y, 2.0));

			if(!point->next)
				continue;

			if(dist < best_dist) {
				found = true;

				best_path = path;
				best_point = point;

				best_dist = dist;
			}
		}
	}

	if(!found)
		return NULL;

	/*
	 * If a path starts with our best point, we don't need to split it
	 */
	if(!best_point->prev)
		return best_path;

	struct point *new = malloc(sizeof(struct point));
	new->x = best_point->x;
	new->y = best_point->y;
	new->next = best_point->next;
	new->prev = NULL;

	best_point->next = NULL;

	return add_path(paths, new);
}

int main(int argc, char **argv) {
	struct path *all_paths = get_paths(stdin);

	printf(preamble);

	double x = 0, y = 0;
	struct path *cur_path;
	while((cur_path = find_near_path(&all_paths, x, y)) != NULL) {
		if(x != cur_path->points->x || y != cur_path->points->y) {
			printf("G00 Z%f\n", ZSAFE);
			printf("G00 X%f Y%f\n", cur_path->points->x, cur_path->points->y);
			printf("G01 Z0 F254\n");
		}

		for(struct point *p = cur_path->points->next; p; p = p->next) {
			x = p->x;
			y = p->y;

			printf("G01 X%f Y%f F300\n", x, y);
		}


		remove_path(&all_paths, cur_path);
	}

	printf(postamble);

	return EXIT_SUCCESS;
}
