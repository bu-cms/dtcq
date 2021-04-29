#include<include/FIFO.h>
#include<interface/Circuit.h>
#include <memory>
#include<stdint.h>
int main(){
    auto circuit = std::make_shared<Circuit>();
    auto fifo = std::make_shared<FIFO<uint64_t>>();
}