#include <utility>

//
// Created by yutianzuo on 2019-03-11.
//

#ifndef TESTPROJ_SIMPLEINTERPROCLOCK_H
#define TESTPROJ_SIMPLEINTERPROCLOCK_H

#include <fcntl.h>
#include <sys/errno.h>
#include <string>
#include <iostream>
#include <unistd.h>

/**
 * a simple inter-process lock using file-lock fcntl
 * an advisory lock, just use to sync among processes
 * in each process DON'T multi-thread invoke try_lock()
 * the purpose is ONLY sync among procs in UNIX LIKE OS
 */
class SimpleInterProcLock final
{
public:
    SimpleInterProcLock(const SimpleInterProcLock &) = delete;

    SimpleInterProcLock(SimpleInterProcLock &&) = delete;

    SimpleInterProcLock &operator=(const SimpleInterProcLock &) = delete;

    SimpleInterProcLock &operator=(SimpleInterProcLock &&) = delete;

    explicit SimpleInterProcLock(std::string str_name) : m_name(std::move(str_name)), m_fd(-1)
    {

    }


    /**
     * file lock is an advisor lock, that means you can also
     * write data in fd when you DON'T get a lock
     * @return
     */
    bool try_lock()
    {
        int fd = ::open(m_name.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (fd != -1)
        {
            m_fd = fd;
            struct flock lock_stru;
            lock_stru.l_len = 0;
            lock_stru.l_start = 0;
            lock_stru.l_type = F_WRLCK;
            lock_stru.l_whence = SEEK_SET;
            int n_ret = ::fcntl(fd, F_SETLK, &lock_stru);
            if (n_ret < 0)
            {
                if (EACCES == errno || EAGAIN == errno)
                {
                    if (::fcntl(fd, F_GETLK, &lock_stru) != -1)
                    {
                        std::cout << "PID=" << lock_stru.l_pid << " owns the lock" << std::endl;
                    }
                    return false; ///exist
                }
                else
                {
                    return false; ////fcntl err
                }
            }
            else
            {
                std::cout << "lock owner pid:" << getpid() << std::endl;
                return true; ////create a inter-process lock;
            }
        }
        else
        {
            return false;
        }
    }

    void release_lock()
    {
        ::close(m_fd);
    }

    ~SimpleInterProcLock()
    {
        release_lock();
    }


    static void execute()
    {
        SimpleInterProcLock lock("test.txt");
        if (lock.try_lock())
        {
            std::cout << "lock got" << std::endl;
            while (true)
            {
                stringxa str_buff;
                std::cin >> str_buff;
                if (str_buff.compare_no_case("exit") == 0)
                {
                    break;
                }
            }
        }
        else
        {
            std::cout << "lock is taken" << std::endl;
        }

    }

private:
    std::string m_name;
    int m_fd;
};

#endif //TESTPROJ_SIMPLEINTERPROCLOCK_H
