#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#define PI 3.142857f

typedef struct {
    float x;
    float y;
    float z;
} vec3d;

typedef struct {
    float x;
    float y;
} vec2d;

typedef struct {
    // i prefer it this way.
    float x0y0;
    float x1y0;
    float x2y0;
    float x0y1;
    float x1y1;
    float x2y1;
    float x0y2;
    float x1y2;
    float x2y2;
} matrix33;

typedef struct {
    int a;
    int b;
    int c;
} tri;

vec3d apply_matrix_transform(
    vec3d vec,
    matrix33 matrix
) {
    vec3d ret = {
        vec.x * matrix.x0y0 + vec.y * matrix.x1y0 + vec.z * matrix.x2y0,
        vec.x * matrix.x0y1 + vec.y * matrix.x1y1 + vec.z * matrix.x2y1,
        vec.x * matrix.x0y2 + vec.y * matrix.x1y2 + vec.z * matrix.x2y2
    };
    return ret;
}

vec3d apply_rotation_transform(
    vec3d vec,
    int mat,
    float rads
) {
    vec3d ret = {0.0, 0.0, 0.0};
    float cos_ = cosf(rads);
    float sin_ = sinf(rads);
    switch (mat) {
        case 0:
            // rotate about X
            ret.x = vec.x;
            ret.y = vec.y * cos_ + vec.z * -sin_;
            ret.z = vec.y * sin_ + vec.z * cos_;
            return ret;
        case 1:
            // rotate about Y
            ret.x = vec.x * cos_ + vec.z * -sin_;
            ret.y = vec.y;
            ret.z = vec.x * sin_ + vec.z * cos_;
            return ret;
        case 2:
            // rotate about Z
            ret.x = vec.x * cos_ + vec.y * -sin_;
            ret.y = vec.x * sin_ + vec.y * cos_;
            ret.z = vec.z;
            return ret;
        default:
            return ret;
    }
}

matrix33 mult_matrices(
    matrix33 a,
    matrix33 b
) {
    // y'all, fuck matrix mutliplication
    matrix33 ret = {
        a.x0y0 * b.x0y0 + a.x1y0 * b.x0y1 + a.x2y0 * b.x0y2, a.x0y0 * b.x1y0 + a.x1y0 * b.x1y1 + a.x2y0 * b.x1y2, a.x0y0 * b.x2y0 + a.x1y0 * b.x2y1 + a.x2y0 * b.x2y2,
        a.x0y1 * b.x0y0 + a.x1y1 * b.x0y1 + a.x2y1 * b.x0y2, a.x0y1 * b.x1y0 + a.x1y1 * b.x1y1 + a.x2y1 * b.x1y2, a.x0y1 * b.x2y0 + a.x1y1 * b.x2y1 + a.x2y1 * b.x2y2,
        a.x0y2 * b.x0y0 + a.x1y2 * b.x0y1 + a.x2y2 * b.x0y2, a.x0y2 * b.x1y0 + a.x1y2 * b.x1y1 + a.x2y2 * b.x1y2, a.x0y2 * b.x2y0 + a.x1y2 * b.x2y1 + a.x2y2 * b.x2y2,
    };
    return ret;
}

vec3d* load_model(
    size_t* size
) {
    FILE* file = fopen("model", "r");
    if (file == NULL) {
        perror("Couldn't find model.");
        return NULL;
    }

    char* model = calloc(1024, sizeof(char));
    size_t bytes_read = fread(model, 1, 1024, file);

    float* model_floats = calloc(512, sizeof(float));
    int model_floats_index = 0;

    char* buffer = calloc(10, sizeof(char));
    int chars_in_buffer = 0;

    for (int i = 0; i < bytes_read; i++) {
        if (chars_in_buffer >= 10) {
            perror("Vertices float lengths were too long. What the fuck are you doing with a +10-digit float?");
            return NULL;
        } else if (model_floats_index >= 512) {
            perror("Over 512 fucking vertices, wtf?");
            return NULL;
        }
        if (model[i] == ' ' || model[i] == '\n') {
            buffer[chars_in_buffer] = '\0';
            model_floats[model_floats_index] = (float)(atof(buffer));
            chars_in_buffer = 0;
            model_floats_index++;
            continue;
        }
        buffer[chars_in_buffer] = model[i];
        chars_in_buffer++;
    }
    if (chars_in_buffer != 0) {
        buffer[chars_in_buffer] = '\0';
        model_floats[model_floats_index] = (float)(atof(buffer));
        chars_in_buffer = 0;
        model_floats_index++;
    }

    vec3d* model_vecs = calloc(513, sizeof(vec3d));
    int idx = 0;
    int vecs_idx = 0;
    vec3d curr_vec;
    for (int i = 0; i < model_floats_index + 1; i++) {
        switch (idx) {
            case 0:
                curr_vec.x = model_floats[i];
                break;
            case 1:
                curr_vec.y = model_floats[i];
                break;
            case 2:
                curr_vec.z = model_floats[i];
                break;
        }
        idx++;
        if (idx == 0 || idx % 3 != 0) {
            continue;
        }
        idx = 0;
        model_vecs[vecs_idx] = curr_vec;
        vecs_idx++;
    }
    *size = (size_t)(vecs_idx);

    free(file);
    free(model);
    free(buffer);
    free(model_floats);
    return model_vecs;
}

tri* load_links(
    size_t* size
) {
    FILE* file = fopen("links", "r");
    if (file == NULL) {
        perror("Couldn't find links.");
        return NULL;
    }

    char* links = calloc(1024, sizeof(char));
    size_t bytes_read = fread(links, 1, 1024, file);

    int* links_ints = calloc(512, sizeof(int));
    int links_idx = 0;

    char* buffer = calloc(10, sizeof(char));
    int buffer_idx = 0;

    for (int i = 0; i < bytes_read; i++) {
        if (buffer_idx >= 10) {
            perror("10 digit links.. DOCTOS, MEDS!!!");
            return NULL;
        } else if (links_idx >= 512) {
            perror("Too many links!!!!");
            return NULL;
        }
        if (links[i] == ' ' || links[i] == '\n') {
            buffer[buffer_idx] = '\0';
            links_ints[links_idx] = atoi(buffer);
            links_idx += 1;
            buffer_idx = 0;
            continue;
        }
        buffer[buffer_idx] = links[i];
        buffer_idx += 1;
    }
    if (buffer_idx != 0) {
        buffer[buffer_idx] = '\0';
        links_ints[links_idx] = atoi(buffer);
        links_idx += 1;
        buffer_idx = 0;
    }

    tri* links_tris = calloc(512, sizeof(tri));
    tri curr_tri;
    int idx = 0;
    int tri_idx = 0;
    for (int i = 0; i < links_idx + 1; i++) {
        switch (idx) {
            case 0:
                curr_tri.a = links_ints[i];
                break;
            case 1:
                curr_tri.b = links_ints[i];
                break;
            case 2:
                curr_tri.c = links_ints[i];
                break;
        }
        idx++;
        if (idx == 0 || idx % 3 != 0) {
            continue;
        }
        idx = 0;
        links_tris[tri_idx] = curr_tri;
        tri_idx++;
    }
    *size = (size_t)(tri_idx);

    free(file);
    free(links);
    free(buffer);
    free(links_ints);
    return links_tris;
}

float radians(
    float degrees
) {
    return degrees * (PI/180.0f);
}

vec2d project(
    vec3d vec
) {
    vec2d ret = {
        roundf(400 + 400 * vec.x / (vec.z + 1)),
        roundf(300 + 400 * vec.y / (vec.z + 1))
    };
    return ret;
}

float dot_product(
    vec3d vec_a,
    vec3d vec_b
) {
    return vec_a.x * vec_b.x + vec_a.y * vec_b.y + vec_a.z * vec_b.z;
}

vec3d cross_product(
    vec3d vec_a,
    vec3d vec_b
) {
    vec3d ret = {
        vec_a.y * vec_b.z - vec_a.z * vec_b.y,
        vec_a.z * vec_b.x - vec_a.x * vec_b.z,
        vec_a.x * vec_b.y - vec_a.y * vec_b.x
    };
    return ret;
}

vec3d sub_vecs(
    vec3d a,
    vec3d b
) {
    vec3d ret = {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
    return ret;
}

float get_magnitude(
    vec3d vec
) {
    return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

int main(int argc, char *argv[]) {
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "3dc");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, "v0.0.0");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, "com.Luxof.3dc");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Luxof");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "application");
    SDL_InitSubSystem(SDL_INIT_VIDEO || SDL_INIT_EVENTS);

    SDL_Window* window = SDL_CreateWindow("3dc", 800, 600, SDL_WINDOW_INPUT_FOCUS);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    vec3d camera_pos = {
        0.0,
        0.0,
        -1.0
    };

    uint8_t running = true;

    float theta = radians(1);
    float co_ = cosf(theta);
    float si_ = sinf(theta);
    matrix33 rot_matrix_x = {
        1.0, 0.0, 0.0,
        0.0, co_, -si_,
        0.0, si_, co_
    };
    matrix33 rot_matrix_y = {
        co_, 0.0, -si_,
        0.0, 1.0, 0.0,
        si_, 0.0, co_
    };
    matrix33 rot_matrix_z = {
        co_, -si_, 0.0,
        si_, co_, 0.0,
        0.0, 0.0, 1.0
    };
    matrix33 rot_ALL = mult_matrices(rot_matrix_z, mult_matrices(rot_matrix_y, rot_matrix_x));

    size_t model_len;
    size_t links_len;
    vec3d* model = load_model(&model_len);
    tri* links = load_links(&links_len);
    if (model == NULL) {
        running = false;
    } else if (links == NULL) {
        running = false;
    }

    while (running) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = 0;
                    break;
                default:
                    break;
            }
        }

        if (!running) {
            break;
        }

        vec2d points[3];
        for (int i = 0; i < links_len; i++) {
            vec3d vertices[3] = {
                model[links[i].a],
                model[links[i].b],
                model[links[i].c]
            };

            vec3d line_a = sub_vecs(vertices[0], vertices[1]);
            vec3d line_b = sub_vecs(vertices[0], vertices[2]);
            vec3d normal = cross_product(line_a, line_b);
            float mag = get_magnitude(normal);
            normal.x = normal.x / mag;
            normal.y = normal.y / mag;
            normal.z = normal.z / mag;

            vec3d cam_to_normal = sub_vecs(vertices[0], camera_pos);
            if (dot_product(cam_to_normal, normal) > 0.0) {
                continue;
            }

            points[0] = project(vertices[0]);
            points[1] = project(vertices[1]);
            points[2] = project(vertices[2]);

            // FILL THEM TRIANGLES IN
            // step 1 find the top and bottom points
            vec2d top = points[0];
            int top_idx = 0;
            vec2d midpoint = points[0];
            int midpoint_idx = 0;
            vec2d bottom = points[2];
            int bottom_idx = 0;
            if (top.y > points[1].y) {
                top = points[1];
                top_idx = 1;
            }
            if (top.y > points[2].y) {
                top = points[2];
                top_idx = 2;
            }
            if (bottom.y < points[1].y) {
                bottom = points[1];
                bottom_idx = 1;
            }
            if (bottom.y < points[2].y) {
                bottom = points[2];
                bottom_idx = 2;
            }

            while (midpoint_idx == top_idx || midpoint_idx == bottom_idx) {
                midpoint_idx++;
                midpoint = points[midpoint_idx];
            }

            float tall_side_x_increment = (bottom.x - top.x) / (bottom.y - top.y);
            float short_sides_x_increments[2] = {
                (midpoint.x - top.x) / (midpoint.y - top.y),
                (bottom.x - midpoint.x) / (bottom.y - midpoint.y)
            };
            float short_sides_x[2] = {
                top.x,
                midpoint.x
            };
            float offset_y = 0.0f;
            int curr_increment = 0;
            for (float y = top.y; y < bottom.y; y++) {
                float calc_y = y - top.y;
                float x1 = top.x + calc_y * tall_side_x_increment;
                float x2 = short_sides_x[curr_increment] + (calc_y - offset_y) * short_sides_x_increments[curr_increment];

                SDL_RenderLine(renderer, x1, y, x2, y);

                if (y == midpoint.y) {
                    curr_increment = 1;
                    offset_y = calc_y;
                }
            }

            SDL_RenderLine(renderer, points[0].x, points[0].y, points[1].x, points[1].y);
            SDL_RenderLine(renderer, points[1].x, points[1].y, points[2].x, points[2].y);
            SDL_RenderLine(renderer, points[2].x, points[2].y, points[0].x, points[0].y);
        }

        vec3d vertex;
        for (int i = 0; i < model_len; i++) {
            vertex = model[i];
            vec3d rotated_vertex = apply_matrix_transform(vertex, rot_ALL);
            model[i] = rotated_vertex;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay((uint32_t)(round(1.0/60.0*1000.0)));
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_PollEvent(NULL);

    SDL_Quit();
    return 0;
}
