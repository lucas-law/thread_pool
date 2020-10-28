#include <iostream>
#include <random>
#include "thread_pool.hpp"

std::random_device rd; // Real random producter
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(-1000, 1000); // Make -1000~1000 numbers that was random.
auto rnd = std::bind(dist, mt);

// Set thread sleep time.
void simulate_hard_computation()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
}

void multiply(const int a, const int b)
{
    simulate_hard_computation();
    const int res = a * b;
    std::cout << a << " * " << b << " = " << res << std::endl;
}

void multiply_output(int &out, const int a, const int b)
{
    simulate_hard_computation();
    out = a * b;
    std::cout << a << " * " << b << " = " << out << std::endl;
}

int multiply_return(const int a, const int b)
{
    simulate_hard_computation();
    const int res = a * b;
    std::cout << a << " * " << b << " = " << res << std::endl;
    return res;
}

void example()
{
    ThreadPool pool(3); 
    pool.initialize();

    // 提交乘法操作，总共30个。
    for (int i = 1; i <= 3; ++i) {
        for (int j = 1; j <= 10; ++j) {
            std::cout << "提交" << i << "*" << j << std::endl;
            pool.submit(multiply, i, j);
        }
    }

    std::cout << "提交" << 5 << "*" << 6 << std::endl;
    // 使用ref传递的输出参数提交函数
    int output_ref = 0;
    auto future1 = pool.submit(multiply_output, std::ref(output_ref), 5, 6);
    std::cout << "提交" << 5 << "*" << 6 << "完成"<< std::endl;
    // 等待乘法输出完成
    future1.get();
    std::cout << "Last operation result is equals to " << output_ref << std::endl;

    // 使用return 参数提交函数
    std::cout << "提交" << 5 << "*" << 3 << std::endl;
    auto future2 = pool.submit(multiply_return, 5, 3);
    std::cout << "提交" << 5 << "*" << 3 << "完成"<< std::endl;
    // 等待乘法输出完成
    int res = future2.get();
    std::cout << "Last operation result is equals to " << res << std::endl;

    // 关闭线程池
    pool.shutdown();

}


void example_1()
{
    ThreadPool pool(2); //开2个线程
    pool.initialize();
    auto worker = []{
        for (int i = 0; i < 10; ++i) {
            std::cout << "worker, thread_id:" << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "worker done.\n";
    };
    pool.submit(worker);
    pool.submit(worker);
    pool.submit(worker);
    pool.submit(worker);
    while (1) {
        std::cout << "main, thread_id:" << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    pool.shutdown();
}

std::mutex g_mutex;
std::condition_variable g_cv;
std::string data;
bool ready = false;
bool processed = false;

void condition_worker_thread()
{
    // 等待main线程发送数据
    std::unique_lock<std::mutex> lock(g_mutex);
    std::cout << "Worker thread is waitting for ready\n";
    g_cv.wait(lock, [] { return ready; });

    // 等待后， 占有锁
    std::cout << "Worker thread is processing data\n";
    data += "after processing";

    std::this_thread::sleep_for(std::chrono::seconds(3));
    // 将数据发送回mian线程
    processed = true;
    std::cout << "Worker thread signals data processing completed\n";
    // 通知前完成手动解锁， 以避免等待线程才被唤醒就阻塞
    lock.unlock();
    g_cv.notify_one();

}

void example_condition_var()
{
    std::thread worker(condition_worker_thread); 
    data = "Example data";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    {
        // 发送数据到worker线程
        std::lock_guard<std::mutex> lock(g_mutex);
        ready = true;
        std::cout << "main() signals data ready for processing\n";
    }
    g_cv.notify_one();
    //等候worker
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        std::cout << "Main thread is waitting for worker processed. " << std::endl;
        g_cv.wait(lock, []{ return processed; });
    }
    std::cout << "Back in main(), data = " << data << std::endl;
    worker.join();
}



int main(int argc, char **argv)
{
    std::cout << "hello, world" << std::endl; 
    int number_of_threads = std::thread::hardware_concurrency();
    std::cout << "max number of threads:" << number_of_threads << std::endl;
    example_1();
    //example_condition_var();
    return EXIT_SUCCESS;
}

