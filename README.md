# SIMD оптимизации на примере отрисовки множества Мандельброта

## About

В данном проекте рассмотрено влияние SIMD инструкций на скорость выполнения трудных вычислений. В качестве задачи для тестирования был выбран рассчёт множества Мандельброта, которое в последствии было отрисовано с помощью библиотеки [SFML](https://www.sfml-dev.org/index.php). Задача была решена тремя способами:

- Просчёт каждой точки отдельно без оптимизаций.
- Просчёт массивов точек, симуляция SIMD инструкций.
- Просчёт точек с использованием SIMD инструкций.

Последний вариант был переписан для получения большей точности изображения, что позволило получать большее увеличение изображения примерно в 2000 раз.

![img](img/image.png "где-то на просторах комплексной плоскости")

Подсчёт точек ведется методом описанным [здесь](https://ru.wikipedia.org/wiki/%D0%9C%D0%BD%D0%BE%D0%B6%D0%B5%D1%81%D1%82%D0%B2%D0%BE_%D0%9C%D0%B0%D0%BD%D0%B4%D0%B5%D0%BB%D1%8C%D0%B1%D1%80%D0%BE%D1%82%D0%B0). Происходит подсчёт двух последовательностей: $\{x_n\}$ и $\{y_n\}$ (действительная и мнимая часть числа, заданного рекурентой $z_{n+1} = z_n^2 + c,\ z_0 = 0,\ c = x_0 + y_0 \cdot i \in C$):

$Re(z_{n+1}) = x_{n+1}={x_{n}}^{2} - {y_{n}}^{2} + x_{0}$

$Im(z_{n+1}) = y_{n+1}=2 \cdot {x_{n}}{y_{n}} + y_{0}$

Подсчёт ведётся до тех пор, пока $|z_n| < 2$ или не достигнется предельное число итераций, определяемое заранее(в нашем случае это было 255). На основе числа проведённых итераций составляется цвет точки с координатами $(x_0, y_0)$.

Для измерения времени использовалась инструкция `rdtsc`, которая возвращает в регистрах `edx:eax` количество тактов с момента последнего сброса процессора. Измерения проводились 100 раз на каждом из 3-ёх запусках и усреднялись.

Все тесты запускались с флагами `-O0 -mavx2` и `-O3 -mavx2`.

## Build

    git clone https://github.com/Gris-a/Mandelbrot
    make

все необходимые исполняемые файлы появятся в папке `executables/`.

## No optimizations

В данном пункте нужно было релизовать рассчёт множества "в лоб", нужно было пройтись по каждому пикселю в двойном цикле и рассчитать его цвет. Код представлен в файле `source/NoSIMD.cpp`. Результаты таковы:

|       | $t_1$                   | $t_2$                   | $t_3$                   | $t$                     |
|:-----:|:-----------------------:|:-----------------------:|:-----------------------:|:-----------------------:|
|  -O0  | $(1133 ± 2) \cdot 10^6$ | $(1131 ± 2) \cdot 10^6$ | $(1133 ± 1) \cdot 10^6$ | $(1132 ± 2) \cdot 10^6$ |
|  -O3  | $(553  ± 3) \cdot 10^6$ | $(554  ± 3) \cdot 10^6$ | $(553  ± 3) \cdot 10^6$ | $(553  ± 3) \cdot 10^6$ |

## SIMD simulation

В данном пункте требовалось попытаться заставить компилятор оптимизировать код, рассчитывая множество не по одному пикселю, а сразу по массиву пикселей, тем самым симулируя поведение SIMD инструкций. Код представлен в файле `source/NoSIMD2.cpp`. Результаты таковы:

|       | $t_1$                   | $t_2$                   | $t_3$                   | $t$                     |
|:-----:|:-----------------------:|:-----------------------:|:-----------------------:|:-----------------------:|
|  -O0  | $(3071 ± 2) \cdot 10^6$ | $(3070 ± 6) \cdot 10^6$ | $(3080 ± 6) \cdot 10^6$ | $(3074 ± 5) \cdot 10^6$ |
|  -O3  | $(245  ± 2) \cdot 10^6$ | $(245  ± 2) \cdot 10^6$ | $(245  ± 3) \cdot 10^6$ | $(245  ± 2) \cdot 10^6$ |

## SIMD

На данном этапе рассчёт множества проводился за счёт встроенных векторных инструкций векторами `[8 × float]`, так как мой процессор поддерживает только `avx2`. Код представлен в файле `source/SIMD.cpp`. Результаты таковы:

|       | $t_1$                   | $t_2$                    | $t_3$                     | $t$                    |
|:-----:|:-----------------------:|:------------------------:|:-------------------------:|:----------------------:|
|  -O0  | $(1814 ± 9) \cdot 10^5$ | $(1813 ±  6) \cdot 10^5$ | $(1820 ± 20) \cdot 10^5$  | $(182 ± 1) \cdot 10^6$ |
|  -O3  | $(773  ± 7) \cdot 10^5$ | $(780  ± 20) \cdot 10^5$ | $(780  ± 30) \cdot 10^5$  | $(78  ± 2) \cdot 10^6$ |

## Conclusion

Как видно из результатов измерений, можно сделать следующие выводы:

- использование SIMD инструкций по сравнению с подсчётом "в лоб" дало прирост скорости в $\frac{553 ± 3}{78 ± 2} = 7.1 ± 0.2$.

- использование массивов и флага `-O3` заставило компилятор использовать SIMD инструкции очень эффективно, что дало прирост скорости в $\frac{553 ± 3}{245 ± 2} = 2.26 ± 0.03$.

Использование SIMD инструкций положительно влияет на скорость выполнения программы. Прирост скорости не соответствует ожиданиям в 8 раз, что можно оправдать тем, что SIMD инструкции являются более трудоёмкими для процессора нежели скалярные. Так же нам приходится выполнять лишние действия при подсчёте, так как вычисления ведутся до тех пор, пока все 8 точек не достигнут условия выхода. Что удивительно, так это невероятный прирост скорости при использовании обычных массивов с флагами компиляции `-mavx2 -O3`, который объясняется тем, что большую часть вычислений компилятор заменил на SIMD инструкции.
