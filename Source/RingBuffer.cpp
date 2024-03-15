/*
  ==============================================================================

    RingBuffer.cpp
    Created: 13 Mar 2024 12:53:41am
    Author:  plumm

  ==============================================================================
*/

#include "RingBuffer.h"

//default constructor
template<typename T>
RingBuffer<T>::RingBuffer() : buffer(0), capacity(0), head(0), tail(0), full(false) {}

template<typename T>
void RingBuffer<T>::write(const T& data) {
    if (full)
        throw std::overflow_error("Buffer is full");

    buffer[head] = data;
    head = (head + 1) % capacity;
    if (head == tail)
        full = true;
}

template<typename T>
T RingBuffer<T>::read() {
    if (is_empty())
        throw std::underflow_error("Buffer is empty");

    T data = buffer[tail];
    tail = (tail + 1) % capacity;
    full = false;
    return data;
}

template<typename T>
bool RingBuffer<T>::is_empty() const {
    return !full && head == tail;
}

template<typename T>
size_t RingBuffer<T>::size() const {
    if (full)
        return capacity;
    return (head - tail + capacity) % capacity;
}


template<typename T>
void RingBuffer<T>::clear() {
    
    std::fill(buffer.begin(), buffer.end(), T{}); 
    head = tail = 0;
    full = false;
}

template<typename T>
void RingBuffer<T>::copyVector(const std::vector<T>& processBuffer) {


    for (int sample = 0; sample < processBuffer.size(); ++sample) {
        buffer[sample] = processBuffer[sample];
    }
}


template<typename T>
void RingBuffer<T>::setSize(size_t bufferSize) {
    buffer.resize(bufferSize);
    capacity = bufferSize;
}

template<typename T>
void RingBuffer<T>::copyArray(const T* processBuffer, size_t bufferSize) {

    for (size_t sample = 0; sample < bufferSize; ++sample) {
        buffer[sample] = processBuffer[sample];
    }
}

template class RingBuffer<float>;