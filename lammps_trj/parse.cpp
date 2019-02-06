#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifndef MAX_LINE_C
#define MAX_LINE_C 0x10000
#endif

typedef struct Point {
    double x, y, z;
    inline double euclidean_distance(const Point& other) {
        double dx = x - other.x, dy = y - other.y, dz = z - other.z;
        return dx * dx + dy * dy + dz * dz;
    }
    inline double euclidean_distance(double xu, double yu, double zu) {
        double dx = x - xu, dy = y - yu, dz = z - zu;
        return dx * dx + dy * dy + dz * dz;
    }
    inline void set(double xu, double yu, double zu) {
        x = xu;
        y = yu;
        z = zu;
    }
} Point;

static int file_line = 0;

char* get_line(FILE* fp, int eof_return = 0) {
    static char buf[MAX_LINE_C];
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        if (ferror(fp)) {
            fprintf(stderr, "I/O error when reading\n");
        } else if (feof(fp)) {
            if (eof_return) {
                return NULL;
            }
            fprintf(stderr, "Unexpected end of file\n");
        } else {
            fprintf(stderr, "Unexpected error occurred when reading\n");
        }
        exit(1);
    }
    ++file_line;
    // printf("%d %s", file_line, buf);
    return buf;
}

int calc_pos(FILE* fp, std::vector<Point>& pos) {
    int n = static_cast<int>(pos.size()), expect_id = 0;
    double square_sum = 0;
    while (n--) {
        const char* buf = get_line(fp);
        int id, type;
        double xu, yu, zu;
        if (sscanf(buf, "%d%d%lf%lf%lf", &id, &type, &xu, &yu, &zu) != 5) {
            fprintf(stderr, "File format error at line %d : %s", file_line, buf);
            return -2;
        }
        // printf("%d %d %f %f %f %s", id, type, xu, yu, zu, buf);
        if (id != expect_id + 1) {
            fprintf(stderr, "Expect id %d but %d at line %d : %s", expect_id, id, file_line, buf);
            return -2;
        }
        Point& prev = pos[expect_id++];
        square_sum += prev.euclidean_distance(xu, yu, zu);
        prev.set(xu, yu, zu);
    }
    return square_sum;
}

int skip_line(FILE* fp, const char* line_id) {
    const char* buf;
    do {
        buf = get_line(fp, 1);
        if (buf == NULL) {
            return -1;
        }
        // printf("skip %d\n", file_line);
    } while (std::string(buf).find(line_id) == std::string::npos);
    // } while (strcmp(get_line(fp), line_id));
    return 0;
}

int get_num(FILE* fp, const char* line_id, int* num) {
    if (skip_line(fp, line_id)) {
        return -1;
    }
    const char* buf = get_line(fp, 1);
    if (buf == NULL) {
        fprintf(stderr, "Unexpected end of file, expect a `NUM` line after line %s\n", line_id);
        return -1;
    }
    if (sscanf(buf, "%d", num) != 1) {
        fprintf(stderr, "Unexpected end of file, expect a `NUM` line after line %s\n", line_id);
        return -1;
    }
    return 0;
}

FILE* get_file(const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);
    if(!fp) {
        perror("File opening failed");
        // exit(EXIT_FAILURE);
        return NULL;
    }
    return fp;
}

int main() {
    const char* timestep_line = "ITEM: TIMESTEP";
    const char* atom_num_line = "ITEM: NUMBER OF ATOMS";
    const char* atom_pos_line = "ITEM: ATOMS id type xu yu zu";
    int timestep, atom_num;
    FILE *fp_in = stdin, *fp_out = stdout;
    fprintf(fp_out, "timestep MSD\n");
    if (get_num(fp_in, timestep_line, &timestep) == 0
        && get_num(fp_in, atom_num_line, &atom_num) == 0) {
        if (atom_num < 0) {
            fprintf(stderr, "NUMBER OF ATOMS expect >=0 but %d", atom_num);
            return -1;
        }
        std::vector<Point> pos(atom_num);
        skip_line(fp_in, atom_pos_line);
        calc_pos(fp_in, pos);
        while (get_num(fp_in, timestep_line, &timestep) == 0
            && get_num(fp_in, atom_num_line, &atom_num) == 0) {
            if (atom_num != static_cast<int>(pos.size())) {
                fprintf(stderr, "NUMBER OF ATOMS expect %d but %d",
                    static_cast<int>(pos.size()), atom_num);
                return -1;
            }
            skip_line(fp_in, atom_pos_line);
            double msd = calc_pos(fp_in, pos);
            if (msd < 0) {
                return -1;
            }
            fprintf(fp_out, "%d %f\n", timestep, msd / atom_num);
        }
    }
    return 0;
}
