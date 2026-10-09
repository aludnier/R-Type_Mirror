/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Zipper.h
*/

#ifndef ZIPPER_H
#define ZIPPER_H

#include "ZipperIterator.h"

namespace RType {

    template <class... Containers>
    class Zipper;


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

        value_type operator*() {
            return this->toValue(_seq);
        }
        value_type operator->() {
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
        bool allSet(std::index_sequence<Is...>) {
            return (... && (*std::get<Is>(this->_current)).has_value());
        }

        template <std::size_t... Is>
        value_type toValue(std::index_sequence<Is...>) {
            return value_type((*std::get<Is>(this->_current)).value()...);
        }

        iterator_tuple _current;
        std::size_t _max;   // compared with _idx to stop the iteration
        std::size_t _idx;

        static constexpr std::index_sequence_for<Containers...> _seq{};
    };

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