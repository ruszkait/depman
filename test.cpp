#include "borrow_checker.hpp"
#include "root_shared_ptr.hpp"
#include <memory>

class DataStoreI
{
public:
    virtual void store(int data);
    virtual bool ready() const;
};

class DataStore : public DataStoreI
{
public:
    void store(int data) override
    {
    }

    bool ready() const override
    {
        return true;
    }
};

class DataSource
{
public:
    DataSource(std::shared_ptr<DataStoreI> data_store)
        : data_store_(std::move(data_store))
    {}

    void produce()
    {
        data_store_->store(42);
    }

private:
    std::shared_ptr<DataStoreI> data_store_;
};


class DataSystem
{
public:
    DataSystem()
        : store_(depman::root_shared_ptr<DataStore>::PolicyBuilder()
                    .premature_destruction_terminates())
        , source_(depman::root_shared_ptr<DataSource>::PolicyBuilder()
                    .premature_destruction_terminates(),
                    store_.get())
    {}

    void run()
    {
        source_.get()->produce();
    }

private:
    depman::root_shared_ptr<DataStore> store_;
    depman::root_shared_ptr<DataSource> source_;
};



int main()
{
    return 0;
}