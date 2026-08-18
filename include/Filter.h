#ifndef FILTER_H
#define FILTER_H

/**
 * Minimalna, lokalna implementacja szablonu ExponentialFilter.
 *
 * Biblioteka "Filter" (autor: Evert Arias), której wymaga EasyButtonTouch
 * z biblioteki EasyButton 2.0.3, nie jest dostępna ani w rejestrze PlatformIO,
 * ani na GitHubie. Ponieważ projekt NIE używa dotykowych przycisków
 * (EasyButtonTouch), dostarczamy tutaj tylko tyle, ile potrzeba do poprawnej
 * kompilacji tego pliku biblioteki.
 *
 * API jest zgodne z tym, czego używa EasyButtonTouch:
 *   ExponentialFilter<long> ADCFilter;
 *   ADCFilter(5, threshold);   // (współczynnik wygładzania, wartość początkowa)
 *   ADCFilter.Filter(value);   // -> przefiltrowana wartość
 *   ADCFilter.Current();       // -> bieżąca wartość
 */

template <typename T>
class ExponentialFilter
{
public:
    ExponentialFilter(float smoothing, T initialValue)
        : _smoothing(smoothing), _current(initialValue), _last(initialValue) {}

    T Filter(T newValue)
    {
        _last = _current;
        _current = static_cast<T>(_smoothing * newValue + (1.0f - _smoothing) * _last);
        return _current;
    }

    T Current() const { return _current; }
    T Last() const { return _last; }

private:
    float _smoothing;
    T _current;
    T _last;
};

#endif // FILTER_H
