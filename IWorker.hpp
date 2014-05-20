#ifndef IWORKER_HPP_INCLUDED
#define IWORKER_HPP_INCLUDED

class IWorker {
public:
    virtual void fn(char[8] identifier) = 0;
};

#endif // IWORKER_HPP_INCLUDED
