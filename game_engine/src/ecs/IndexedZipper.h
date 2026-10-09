/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** IndexedZipper.h
*/

#ifndef INDEXEDZIPPER_H
#define INDEXEDZIPPER_H

#include "Zipper.h"

namespace RType {
    template<class... Containers>
    class IndexedZipperIterator {
    public:
        using base_iterator = ZipperIterator<Containers...>;

        using value_type = decltype(std::tuple_cat(
            std::declval<std::tuple<std::size_t> >(),
            std::declval<typename base_iterator::value_type>()));
        using reference = value_type;
        using pointer = void;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        explicit IndexedZipperIterator(base_iterator const &it) : _it(it) {
        }

        IndexedZipperIterator(IndexedZipperIterator const &) = default;

        IndexedZipperIterator &operator++() {
            ++this->_it;
            return *this;
        }

        IndexedZipperIterator operator++(int) {
            IndexedZipperIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        value_type operator*() const {
            return std::tuple_cat(std::tuple<std::size_t>(this->_it.index()), *this->_it);
        }

        value_type operator->() const { return **this; }

        friend bool operator==(IndexedZipperIterator const &lhs, IndexedZipperIterator const &rhs) {
            return lhs._it == rhs._it;
        }

        friend bool operator!=(IndexedZipperIterator const &lhs, IndexedZipperIterator const &rhs) {
            return !(lhs == rhs);
        }

    private:
        base_iterator _it;
    };

    template<class... Containers>
    class IndexedZipper {
    public:
        using iterator = IndexedZipperIterator<Containers...>;

        explicit IndexedZipper(Containers &... containers) : _zipper(containers...) {
        }

        iterator begin() { return iterator(this->_zipper.begin()); }
        iterator end() { return iterator(this->_zipper.end()); }

    private:
        Zipper<Containers...> _zipper;
    };
}

#endif
