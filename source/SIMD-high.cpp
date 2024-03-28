#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include "SFML/System.hpp"

#include <immintrin.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

const unsigned SCREEN_WIDTH  = 400;
const unsigned SCREEN_HEIGHT = 400;

const unsigned PIXELS_PER_OFFSET = 20;
const double MAX_ZERO_OFFSET     = 2;

inline void ProcessEvent(sf::RenderWindow &window, sf::Event &event, double &x_rend, double &y_rend, double &delta, bool &to_render);
inline int64_t RenderMandelbrot(sf::Uint8 *pixels, double x_rend, double y_rend, double delta);
inline void DrawMandelbrot(sf::RenderWindow &window, sf::Uint8 *pixels);

inline int64_t TimeCounter(void);
void TestSIMDHigh(sf::Uint8 *pixels, double x_rend, double y_rend, double delta);

int main(void)
{
// ================================================================================================================================================================================
    double ratio       = (double)SCREEN_HEIGHT / (double)SCREEN_WIDTH;
    double coefficient = (SCREEN_WIDTH > SCREEN_HEIGHT) ? SCREEN_WIDTH : SCREEN_HEIGHT;

    double delta  = 2 * MAX_ZERO_OFFSET / coefficient;
    double x_rend = -MAX_ZERO_OFFSET;
    double y_rend = MAX_ZERO_OFFSET * ratio;
// ================================================================================================================================================================================
    sf::Uint8 *pixels = (sf::Uint8 *)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, 4 * sizeof(sf::Uint8));
    memset(pixels, 255, (SCREEN_WIDTH * SCREEN_HEIGHT) * (4 * sizeof(sf::Uint8)));
// ================================================================================================================================================================================
#ifndef RENDER
    TestSIMDHigh(pixels, x_rend, y_rend, delta);
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

inline void ProcessEvent(sf::RenderWindow &window, sf::Event &event, double &x_rend, double &y_rend, double &delta, bool &to_render)
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

inline int64_t RenderMandelbrot(sf::Uint8 *pixels, double x_rend, double y_rend, double delta)
{
#ifndef RENDER
    int64_t start = TimeCounter();
#endif

    static const unsigned N_ITERATIONS = 1023;

    static const __v4df MAX_ZERO_OFFSET2_V = _mm256_set1_pd(MAX_ZERO_OFFSET * MAX_ZERO_OFFSET);
    static const __v4df SHIFT_V            = _mm256_set_pd(3, 2, 1, 0);

    __v4df delta_v         = _mm256_set1_pd(delta);
    __v4df delta_v_shifted = SHIFT_V * delta_v;

    size_t pix_arr_pos = 0;

    for(unsigned y_pos = 0; y_pos < SCREEN_HEIGHT; y_pos += 1)
    {
        for(unsigned x_pos = 0; x_pos < SCREEN_WIDTH; x_pos += 4)
        {
            __v4df y_0 = _mm256_set1_pd(y_rend - y_pos * delta);
            __v4df x_0 = x_rend + delta_v_shifted + x_pos * delta_v;

            __v4df x_n = {};
            __v4df y_n = {};

            __v4di n = {};
            for(volatile unsigned i = 0; i < N_ITERATIONS; i++)
            {
                __v4df x2 = x_n * x_n;
                __v4df y2 = y_n * y_n;
                __v4df xy = x_n * y_n;

                __v4df cmp = ((x2 + y2) < MAX_ZERO_OFFSET2_V);

                unsigned mask = _mm256_movemask_pd(cmp);
                if(mask == 0) break;

                n -= reinterpret_cast<__v4di>(cmp);

                x_n = x2 - y2 + x_0;
                y_n = xy + xy + y_0;
            }

#ifdef RENDER
            int64_t *n_p = (int64_t *)&n;
            for(unsigned i = 0; i < 4; i++, pix_arr_pos += 4)
            {
                sf::Uint8 color = *(n_p++);
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
    static sf::Sprite sprite;
    static sf::Texture texture;

    texture.create(SCREEN_WIDTH, SCREEN_HEIGHT);
    texture.update(pixels);

    sprite.setTexture(texture);

    window.clear(sf::Color::Black);
    window.draw(sprite);
    window.display();
}

void TestSIMDHigh(sf::Uint8 *pixels, double x_rend, double y_rend, double delta)
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