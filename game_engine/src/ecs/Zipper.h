/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Zipper.h
*/

#ifndef ZIPPER_H
#define ZIPPER_H

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <utility>
#include <iterator>

namespace RType {

    template <class... Containers>
    class Zipper;


    /**
     * @brief Input iterator that walks multiple container iterators in lockstep.
     *
     * ZipperIterator holds a tuple of iterators (one per Container). Dereferencing
     * the iterator returns a tuple of references to the contained values
     * (the contained types are expected to be std::optional<T>; see note).
     *
     * The iterator advances all underlying iterators together. Iteration
     * stops after the minimum container size is reached. When advancing, the
     * iterator will skip positions where one or more optionals are not set
     * (i.e. std::optional::has_value() == false) until it finds a position
     * where all containers have a value, or the maximum iteration index is
     * reached.
     *
     * Preconditions / Notes:
     * - Dereferencing (operator* / operator->) calls std::optional::value()
     *   on each underlying optional. If any optional is disengaged, this will
     *   throw std::bad_optional_access. Callers should expect this behaviour
     *   or ensure that the iterator points to a position where all optionals
     *   are set.
     *
     * @tparam Containers Types of the container-like objects being zipped.
     */
    template <class... Containers>
    class ZipperIterator {
        template <class Container>
        using iterator_t = decltype(std::declval<Container &>().begin());

        template <class Container>
        using it_reference_t = iterator_t<Container>::reference;

    public:
        using value_type = std::tuple<std::remove_reference_t<decltype(std::declval<it_reference_t<Containers>>().value())> &...>;
        using reference = value_type;
        using pointer = void;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using iterator_tuple = std::tuple<iterator_t<Containers>...>;

        friend class Zipper<Containers...>;

        ZipperIterator(iterator_tuple const &it_tuple, std::size_t max) : _current(it_tuple), _max(max), _idx(0) {
            if (this->_idx < this->_max && !this->allSet(_seq)) {
                this->incrementAll(_seq);
            }
        }

        ZipperIterator(ZipperIterator const &z) = default;

        ZipperIterator operator++(int){
            ZipperIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        ZipperIterator &operator++() {
            this->incrementAll(_seq);
            return *this;
        }

        value_type operator*() const {
            return this->toValue(_seq);
        }

        value_type operator->() const {
            return this->toValue(_seq);
        }

        [[nodiscard]] std::size_t index() const {
            return this->_idx;
        }

        friend bool operator==(ZipperIterator const &lhs, ZipperIterator const &rhs) {
            return lhs._current == rhs._current;
        }

        friend bool operator!=(ZipperIterator const &lhs, ZipperIterator const &rhs) {
            return !(lhs == rhs);
        }

    private:
        template <std::size_t... Is>
        void incrementAll(std::index_sequence<Is...>) {
            do {
                (++std::get<Is>(this->_current), ...);
                ++this->_idx;
            } while (this->_idx < this->_max && !this->allSet(_seq));

        }

        template <std::size_t... Is>
        bool allSet(std::index_sequence<Is...>) const {
            return (... && (*std::get<Is>(this->_current)).has_value());
        }

        template <std::size_t... Is>
        value_type toValue(std::index_sequence<Is...>) const {
            return value_type((*std::get<Is>(this->_current)).value()...);
        }

        iterator_tuple _current;
        std::size_t _max;   // compared with _idx to stop the iteration
        std::size_t _idx;

        static constexpr std::index_sequence_for<Containers...> _seq{};
    };

    /**
     * @brief Utility that zips several containers into a single iterable view.
     *
     * Zipper<Containers...> provides an input-range-like object whose
     * iterators produce tuples containing references to the values held in the
     * underlying containers. The zipped range length is the minimum of the
     * sizes of the provided containers.
     *
     * Typical usage: iterate two or more component storages in parallel to
     * access components that belong to the same entity index.
     *
     * @tparam Containers Container types (must provide begin(), end(), size()).
     */
    template <class ...Containers>
    class Zipper {
    public:
        using iterator = ZipperIterator<Containers...>;
        using iterator_tuple = iterator::iterator_tuple;

        explicit Zipper(Containers &... containers) : _begin(std::make_tuple(containers.begin()...)),
                                                      _end(_computeEnd(containers...)),
                                                      _size(_computeSize(containers...)) {
        }

        iterator begin() {
            return iterator(_begin, _size);
        }

        iterator end() {
            return iterator(_end, 0);
        }
    private:
        static size_t _computeSize(Containers &... containers) {
            return std::min({static_cast<std::size_t>(containers.size())...});
        }

        static iterator_tuple _computeEnd(Containers &... containers) {
            auto const size = static_cast<std::ptrdiff_t>(_computeSize(containers...));
            return iterator_tuple((containers.begin() + size)...);
        }

        iterator_tuple _begin;
        iterator_tuple _end;
        size_t _size;
    };
}

#endif