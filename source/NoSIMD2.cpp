#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "SFML/System.hpp"

#include <immintrin.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

const unsigned VECTOR_SZ = 8;

const unsigned SCREEN_WIDTH  = 1920;
const unsigned SCREEN_HEIGHT = 1080;

const unsigned PIXELS_PER_OFFSET = 20;
const float MAX_ZERO_OFFSET      = 2;

inline void ProcessEvent(sf::RenderWindow &window, sf::Event &event, float &x_rend, float &y_rend, float &delta, bool &to_render);
inline int64_t RenderMandelbrot(sf::Uint8 *pixels, float x_rend, float y_rend, float delta);
inline void DrawMandelbrot(sf::RenderWindow &window, sf::Uint8 *pixels);

inline int64_t TimeCounter(void);
void TestNoSIMD2(sf::Uint8 *pixels, float x_rend, float y_rend, float delta);

inline void vset1(float vec[VECTOR_SZ], float val);
inline void vadd(float result[VECTOR_SZ], float vec1[VECTOR_SZ], float vec2[VECTOR_SZ]);
inline void vsub(float result[VECTOR_SZ], float vec1[VECTOR_SZ], float vec2[VECTOR_SZ]);
inline void vmul(float result[VECTOR_SZ], float vec1[VECTOR_SZ], float vec2[VECTOR_SZ]);

int main(void)
{
// ================================================================================================================================================================================
    float ratio       = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
    float coefficient = (SCREEN_WIDTH > SCREEN_HEIGHT) ? SCREEN_WIDTH : SCREEN_HEIGHT;

    float delta  = 2 * MAX_ZERO_OFFSET / coefficient;
    float x_rend = -MAX_ZERO_OFFSET;
    float y_rend = MAX_ZERO_OFFSET * ratio;
// ================================================================================================================================================================================
    sf::Uint8 *pixels = (sf::Uint8 *)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, 4 * sizeof(sf::Uint8));
    memset(pixels, 255, (SCREEN_WIDTH * SCREEN_HEIGHT) * (4 * sizeof(sf::Uint8)));
// ================================================================================================================================================================================
#ifndef RENDER
    TestNoSIMD2(pixels, x_rend, y_rend, delta);
#else
    bool to_render = true;

    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "");
    do {
        sf::Event event;
        while(window.pollEvent(event))
        {
            ProcessEvent(window, event, x_rend, y_rend, delta, to_render);
        }

        if(!to_render) continue;

        RenderMandelbrot(pixels, x_rend, y_rend, delta);
        DrawMandelbrot(window, pixels);

        to_render = false;

    } while(window.isOpen());
#endif
// ================================================================================================================================================================================
    free(pixels);

    return EXIT_SUCCESS;
// ================================================================================================================================================================================
}

inline int64_t TimeCounter(void)
{
    int64_t result = 0;

    asm volatile
    (
        ".intel_syntax noprefix\n\t"
        "rdtsc\n\t"
        "shl rdx, 32\n\t"
        "add rax, rdx\n\t"
        "mov %0, rax\n\t"
        ".att_syntax prefix\n\t"
        : "=r"(result)
        :
        : "%rdx", "%rax"
    );

    return result;
}

inline void ProcessEvent(sf::RenderWindow &window, sf::Event &event, float &x_rend, float &y_rend, float &delta, bool &to_render)
{
    switch(event.type)
    {
        case sf::Event::Closed:
        {
            window.close();
            return;
        }
        case sf::Event::Resized:
        {
            to_render = true;
            window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
            return;
        }
        case sf::Event::KeyPressed:
        {
            to_render = true;
            switch(event.key.code)
            {
                case sf::Keyboard::Left:
                {
                    x_rend -= PIXELS_PER_OFFSET * delta;
                    return;
                }
                case sf::Keyboard::Right:
                {
                    x_rend += PIXELS_PER_OFFSET * delta;
                    return;
                }
                case sf::Keyboard::Up:
                {
                    y_rend += PIXELS_PER_OFFSET * delta;
                    return;
                }
                case sf::Keyboard::Down:
                {
                    y_rend -= PIXELS_PER_OFFSET * delta;
                    return;
                }
                case sf::Keyboard::Dash:
                {
                    x_rend -= delta * (SCREEN_WIDTH  / 2);
                    y_rend += delta * (SCREEN_HEIGHT / 2);

                    delta *= 2;

                    return;
                }
                case sf::Keyboard::Equal:
                {
                    delta /= 2;

                    x_rend += delta * (SCREEN_WIDTH  / 2);
                    y_rend -= delta * (SCREEN_HEIGHT / 2);

                    return;
                }
            }
        }
    }
}

inline int64_t RenderMandelbrot(sf::Uint8 *pixels, float x_rend, float y_rend, float delta)
{
#ifndef RENDER
    int64_t start = TimeCounter();
#endif

    static const unsigned N_ITERATIONS = 255;

    static float SHIFT_V[VECTOR_SZ] = {0, 1, 2, 3, 4, 5, 6, 7};

    float delta_v[VECTOR_SZ]         = {}; vset1(delta_v, delta);
    float delta_v_shifted[VECTOR_SZ] = {}; vmul(delta_v_shifted, SHIFT_V, delta_v);
    float packed_adj_v[VECTOR_SZ]    = {}; vset1(packed_adj_v, VECTOR_SZ * delta);

    size_t pix_arr_pos = 0;

    float y_0[VECTOR_SZ] = {}; vset1(y_0, y_rend);
    for(unsigned y_pos = 0; y_pos < SCREEN_HEIGHT; y_pos += 1, vsub(y_0, y_0, delta_v))
    {
        float x_0[VECTOR_SZ] = {}; vset1(x_0, x_rend);
        vadd(x_0, x_0, delta_v_shifted);
        for(unsigned x_pos = 0; x_pos < SCREEN_WIDTH; x_pos += VECTOR_SZ, vadd(x_0, x_0, packed_adj_v))
        {
            float x_n[VECTOR_SZ] = {};
            float y_n[VECTOR_SZ] = {};

            unsigned n[VECTOR_SZ] = {};
            for(volatile unsigned i = 0; i < N_ITERATIONS; i++)
            {
                float x2[VECTOR_SZ] = {}; vmul(x2, x_n, x_n);
                float y2[VECTOR_SZ] = {}; vmul(y2, y_n, y_n);
                float xy[VECTOR_SZ] = {}; vmul(xy, x_n, y_n);

                float r2[VECTOR_SZ] = {}; vadd(r2, x2, y2);

                unsigned n_left = 0;
                for(unsigned j = 0; j < VECTOR_SZ; j++)
                {
                    if(r2[j] < MAX_ZERO_OFFSET * MAX_ZERO_OFFSET)
                    {
                        n[j]++;
                        n_left++;
                    }
                }
                if(n_left == 0) break;

                vsub(x_n, x2, y2); vadd(x_n, x_n, x_0);
                vadd(y_n, xy, xy); vadd(y_n, y_n, y_0);
            }

#ifdef RENDER
            for(unsigned i = 0; i < VECTOR_SZ; i++, pix_arr_pos += 4)
            {
                sf::Uint8 color = n[i];
                pixels[pix_arr_pos + 0] = color;
                pixels[pix_arr_pos + 1] = color;
                pixels[pix_arr_pos + 2] = color * 32;
            }
#endif
        }
    }

#ifndef RENDER
    int64_t end = TimeCounter();
    return (end - start);
#endif

    return 0;
}

inline void DrawMandelbrot(sf::RenderWindow &window, sf::Uint8 *pixels)
{
    static sf::Texture texture;
    static sf::Sprite sprite;

    texture.create(SCREEN_WIDTH, SCREEN_HEIGHT);
    texture.update(pixels);

    sprite.setTexture(texture);

    window.clear(sf::Color::Black);
    window.draw(sprite);
    window.display();
}

void TestNoSIMD2(sf::Uint8 *pixels, float x_rend, float y_rend, float delta)
{
    const size_t N_TESTS = 100;
    int64_t results[N_TESTS] = {};

    double result_time = 0;
    double error       = 0;

    for(size_t i = 0; i < N_TESTS; i++)
    {
        int64_t delta_time = RenderMandelbrot(pixels, x_rend, y_rend, delta);
        results[i]   = delta_time;
        result_time += (double)delta_time;
    }
    result_time /= (double)N_TESTS;

    for(size_t i = 0; i < N_TESTS; i++)
    {
        int64_t abs_err = results[i] - result_time;
        error += (double)abs_err * (double)abs_err;
    }
    error = sqrt(error / (double)N_TESTS);
    double exp = pow(10, (int)log10(error));

    error       = round(error       / exp) * exp;
    result_time = round(result_time / exp) * exp;
    printf("%lg ± %lg\n", result_time, error);
}

inline void vset1(float vec[VECTOR_SZ], float val)
{
    for(unsigned i = 0; i < VECTOR_SZ; i++)
    {
        vec[i] = val;
    }
}

inline void vadd(float result[VECTOR_SZ], float vec1[VECTOR_SZ], float vec2[VECTOR_SZ])
{
    for(unsigned i = 0; i < VECTOR_SZ; i++)
    {
        result[i] = vec1[i] + vec2[i];
    }
}

inline void vsub(float result[VECTOR_SZ], float vec1[VECTOR_SZ], float vec2[VECTOR_SZ])
{
    for(unsigned i = 0; i < VECTOR_SZ; i++)
    {
        result[i] = vec1[i] - vec2[i];
    }
}

inline void vmul(float result[VECTOR_SZ], float vec1[VECTOR_SZ], float vec2[VECTOR_SZ])
{
    for(unsigned i = 0; i < VECTOR_SZ; i++)
    {
        result[i] = vec1[i] * vec2[i];
    }
}