#ifndef SYNCHRONIZEDINPUT_H_
#define SYNCHRONIZEDINPUT_H_

#include <queue>
#include <boost/thread.hpp>

template <typename T>

// Queue class that has thread synchronisation
class SynchronizedInput
{

public:
// Add one line of the psd extract file to the queue and notify others
void Enqueue(const T& line)
{
// Acquire lock on the queue
boost::unique_lock<boost::mutex> lock(m_mutex);

// Add the data to the queue
m_queue.push(line);

// Notify others that data is ready
m_cond.notify_one();

} // Lock is automatically released here

// Get data from the queue. Wait for data if not available
T Dequeue()
{

// Acquire lock on the queue
boost::unique_lock<boost::mutex> lock(m_mutex);

// When there is no data, wait till someone fills it.
// Lock is automatically released in the wait and obtained
// again after the wait
while (m_queue.size()==0) m_cond.wait(lock);

// Retrieve the data from the queue
T result=m_queue.front(); m_queue.pop();
return result;

} // Lock is automatically released here

bool IsEmpty()
{
	return m_queue.empty();
}

private:
std::queue<T> m_queue; // Use STL queue to store lines of psd file
boost::mutex m_mutex; // The mutex to synchronise on
boost::condition_variable m_cond; // The condition to wait for

};

#endif /* SYNCHRONIZEDINPUT_H_*/
