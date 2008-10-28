#ifndef DEBUG_COUNTER_H
#define DEBUG_COUNTER_H

template<typename T>
struct DebugCounter {
	static int value;
};

template<typename T>
int DebugCounter<T>::value = 0;

#define DEBUG_COUNTER(name) \
	DebugCounter<struct DebugCounter_tag_##name>::value

#endif
