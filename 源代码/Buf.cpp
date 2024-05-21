#include "Buf.h"

Buf::Buf()
{
	this->b_flags = 0;
	this->b_forw =nullptr;
	this->b_back = nullptr;
	this->b_wcount = 0;
	this->b_addr = nullptr;
	this->b_blkno = -1;
}

Buf::~Buf()
{

}
