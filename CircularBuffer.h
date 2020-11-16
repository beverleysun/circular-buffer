#pragma once

#include <stdint.h>
#include <boost/thread.hpp>
#include <iostream>
#include <string>

template <typename T, uint32_t max>
class CircularBuffer {
private:
    boost::mutex m_Mutex;
    boost::condition_variable m_DataCond;

    enum {
        MAX_SIZE = max
    };

    T m_Buffer[MAX_SIZE];
    uint32_t m_WriteOffset;
    uint32_t m_ReadOffset;
    bool full;

    bool bufferEmpty() const {
        return m_WriteOffset == m_ReadOffset && !full;
    }
    
public:

    CircularBuffer() : m_WriteOffset(0), m_ReadOffset(0), full(false) {
    }

    ~CircularBuffer() {
        delete m_Buffer;
    };
    
    std::string valuesAsString() {
    	std::string values = "";
    	if (m_ReadOffset > m_WriteOffset || full) {
    		for (int i=m_ReadOffset; i < MAX_SIZE; i++) {
    			values = values + m_Buffer[i] + ",";
    		}
    		
    		for (int i=0; i < m_WriteOffset; i++) {
    			values = values + m_Buffer[i] + ",";
    		}
    		
    	} else {
    		for (int i=m_ReadOffset; i < m_WriteOffset; i++){
    			values = values + m_Buffer[i] + ",";
    		}
    	}
    	return values;
    }

    void push(T val) {

        boost::unique_lock<boost::mutex> guard(m_Mutex);
        m_DataCond.wait(guard, [this]{return !full && MAX_SIZE !=0;}); // Wait until the buffer isn't full
        m_Buffer[m_WriteOffset++] = val; // Push to buffer

        if (m_WriteOffset == MAX_SIZE) {
            m_WriteOffset = 0;
        }

        if (m_WriteOffset == m_ReadOffset) {
            full = true;
        }

        m_DataCond.notify_one(); // Notify other thread that something was added to buffer
    }

    T pull() {
        boost::unique_lock<boost::mutex> guard(m_Mutex);
		m_DataCond.wait(guard, [this]{return !bufferEmpty() && MAX_SIZE !=0;}); // Wait until the buffer isn't empty
        T obj = m_Buffer[m_ReadOffset++]; // pull from buffer
        
        if (m_ReadOffset == MAX_SIZE) {
            m_ReadOffset = 0;
        }

        full = false;
        
        m_DataCond.notify_one(); // Notify other thread that something was removed from buffer
        
        return obj;
    }
};
