import 序列化内存模型;
#include"序列化内存模型.h"
extern "C"
{
	序列化内存模型_接口 序列化内存模型_管理器* 序列化内存模型_new(bool(*分配器函数)(void*, size_t*, char**), void* 分配器上下文)
	{
		return reinterpret_cast<序列化内存模型_管理器*>(new 序列化内存模型::管理器<>([分配器函数, 分配器上下文](size_t& 空间, char*& 头指针) {return 分配器函数(分配器上下文, &空间, &头指针); }));
	}
	序列化内存模型_接口 void 序列化内存模型_delete(序列化内存模型_管理器* 管理器)
	{
		delete reinterpret_cast<序列化内存模型::管理器<>*>(管理器);
	}
	序列化内存模型_接口 序列化内存模型_内存句柄 序列化内存模型_allocate(序列化内存模型_管理器* 管理器, size_t 字节数)
	{
		return reinterpret_cast<序列化内存模型_内存句柄>(reinterpret_cast<序列化内存模型::管理器<>*>(管理器)->allocate<char>(字节数));
	}
	序列化内存模型_接口 void 序列化内存模型_reallocate(序列化内存模型_管理器* 管理器, 序列化内存模型_内存句柄 句柄, size_t 字节数)
	{
		reinterpret_cast<序列化内存模型::管理器<>*>(管理器)->reallocate(reinterpret_cast<序列化内存模型::内存句柄<char>>(句柄), 字节数);
	}
	序列化内存模型_接口 void 序列化内存模型_deallocate(序列化内存模型_管理器* 管理器, 序列化内存模型_内存句柄 句柄)
	{
		reinterpret_cast<序列化内存模型::管理器<>*>(管理器)->deallocate(reinterpret_cast<序列化内存模型::内存句柄<char>>(句柄));
	}
	序列化内存模型_接口 void* 序列化内存模型_to_address(序列化内存模型_管理器* 管理器, 序列化内存模型_内存句柄 句柄)
	{
		return reinterpret_cast<void*>(reinterpret_cast<序列化内存模型::管理器<>*>(管理器)->to_address(reinterpret_cast<序列化内存模型::内存句柄<char>>(句柄)));
	}
}