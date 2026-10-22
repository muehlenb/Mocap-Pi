#include <type_traits>
#include <set>

namespace Utils {
    /**
     * Returns the value in the sorted that that is closest to the given value.
     */

    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    T findClosest(const std::set<T>& sortedSet, T value) {
        auto it = sortedSet.lower_bound(value);

        if (it == sortedSet.begin())
            return *it;

        if (it == sortedSet.end())
            return *std::prev(it);

        auto prev_it = std::prev(it);

        if (std::abs(*it - value) < std::abs(*prev_it - value))
            return *it;
        else
            return *prev_it;
    }


}
