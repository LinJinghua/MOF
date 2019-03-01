#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <utility>
#include <vector>

#ifndef MAX_LINE_C
#define MAX_LINE_C 0x10000
#endif

#define ALIAS_TEMPLATE_FUNCTION(identifier, function_name, ...)     \
template <class... T>                                               \
inline auto identifier(T&&... t) {                                  \
    return function_name<__VA_ARGS__>(std::forward<T>(t)...);       \
}

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

template <int _EOF_Return>
char* get_line_(FILE* fp) {
    static char buf[MAX_LINE_C];
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        if (feof(fp)) {
            if (_EOF_Return) {
                return NULL;
            }
            fprintf(stderr, "Unexpected end of file\n");
        } else if (ferror(fp)) {
            fprintf(stderr, "I/O error when reading\n");
        } else {
            fprintf(stderr, "Unexpected error occurred when reading\n");
        }
        exit(1);
    }
    ++file_line;
    // printf("%d %s", file_line, buf);
    return buf;
}

ALIAS_TEMPLATE_FUNCTION(get_line_eof, get_line_, 1)
ALIAS_TEMPLATE_FUNCTION(get_line, get_line_, 0)

template <int _Update>
double get_pos_(FILE* fp, std::vector<Point>& pos) {
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
            fprintf(stderr, "Expect id %d but %d at line %d : %s",
                expect_id, id, file_line, buf);
            return -2;
        }
        Point& prev = pos[expect_id++];
        if (!_Update) {
            square_sum += prev.euclidean_distance(xu, yu, zu);
        } else {
            prev.set(xu, yu, zu);
        }
    }
    return square_sum;
}

ALIAS_TEMPLATE_FUNCTION(set_pos, get_pos_, 1)
ALIAS_TEMPLATE_FUNCTION(calc_pos, get_pos_, 0)

const char* skip_blank(const char* str) {
    while (*str && std::isspace(static_cast<unsigned char>(*str))) {
      ++str;
    }
    return str;
}

int strcmp_blank(const char* blank, const char* str) {
    blank = skip_blank(blank);
    int len = strlen(str);
    return strncmp(blank, str, len) || *skip_blank(blank + len);
}

int skip_line(FILE* fp, const char* line_id) {
    const char* buf;
    do {
        buf = get_line_eof(fp);
        if (!buf) {
            return -1;
        }
        // printf("skip %d %s\n", file_line, buf);
    } while (strcmp_blank(buf, line_id));
    return 0;
}

int get_num(FILE* fp, const char* line_id, int* num) {
    if (skip_line(fp, line_id)) {
        return -1;
    }
    const char* buf = get_line_eof(fp);
    if (!buf || sscanf(buf, "%d", num) != 1) {
        fprintf(stderr,
            "Unexpected end of file, expect a `NUM` line after line %s\n",
            line_id);
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

int process(FILE* fp_in, FILE* fp_out) {
    const char* timestep_line = "ITEM: TIMESTEP";
    const char* atom_num_line = "ITEM: NUMBER OF ATOMS";
    const char* atom_pos_line = "ITEM: ATOMS id type xu yu zu";
    int timestep, atom_num;
    fprintf(fp_out, "timestep MSD\n");
    if (!get_num(fp_in, timestep_line, &timestep)
        && !get_num(fp_in, atom_num_line, &atom_num)
        && !skip_line(fp_in, atom_pos_line)) {
        if (atom_num < 0) {
            fprintf(stderr, "NUMBER OF ATOMS expect >=0 but %d", atom_num);
            return -1;
        }
        std::vector<Point> pos(atom_num);
        if (set_pos(fp_in, pos) < .0) {
            return -1;
        }
        while (!get_num(fp_in, timestep_line, &timestep)
            && !get_num(fp_in, atom_num_line, &atom_num)
            && !skip_line(fp_in, atom_pos_line)) {
            if (atom_num != static_cast<int>(pos.size())) {
                fprintf(stderr, "NUMBER OF ATOMS expect %d but %d",
                    static_cast<int>(pos.size()), atom_num);
                return -1;
            }
            double msd = calc_pos(fp_in, pos);
            if (msd < .0) {
                return -1;
            }
            fprintf(fp_out, "%d %f\n", timestep, msd / atom_num);
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    return process(stdin, stdout);
}
