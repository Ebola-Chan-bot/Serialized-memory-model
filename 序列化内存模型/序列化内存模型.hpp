#include<functional>
namespace 序列化内存模型
{
	//你可以把内存句柄本身作为值保存在容器中。重新载入后该句柄仍然有效，但指向的真实内存地址可能已经改变。
	template<typename T>
	using 内存句柄 = T*;
	using size_type = size_t;
	struct 管理器
	{
		const std::function<bool (size_type&,char*&)>分配器;
		/*构造管理器需要用户提供一个底层字节序列特定的分配器。分配器可能被多次调用，应当保证不改变底层字节序列。分配器应当分配不少于输入空间参数值的字节数，并将实际分配的字节数写入空间参数，分配的内存指针写入头指针参数。最后返回一个bool，指示分配的空间是否曾经被管理器写入过：若true，则管理器将可以读取以前保存的内容；否则将覆盖创建新的内容。
		管理器将获得分配器的所有权，管理器删除时分配器也删除。用户应当在分配器的删除过程中负责释放分配的资源。
		*/
		管理器(std::function<bool (size_type& 空间,char*&头指针)>&&分配器) :分配器(std::move(分配器)),空间(sizeof(分配头))
		{
			if (分配器(空间, 头指针))
			{
				const size_type 至少空间 = 分配头指针->分配块空间 * sizeof(分配块) + sizeof(分配头);
				if (至少空间 > 空间)
				{
					空间 = 至少空间 * 2;
					分配器(空间, 头指针);
				}
			}
			else
				*分配头指针 = { 0,0 };
		}
		//返回的引用只能临时使用。对管理器的任何操作都可能导致引用失效。
		template<typename T>
		T& operator*(内存句柄<T>句柄)
		{
			return *to_address(句柄);
		}
		//返回的指针只能临时使用。对管理器的任何操作都可能导致指针失效。
		template<typename T>
		T* to_address(内存句柄<T>句柄)
		{
			const 分配块& 分配 = reinterpret_cast<分配块*>(分配头指针 + 1)[reinterpret_cast<size_type>(句柄)];
			const size_type 至少空间 = 分配.偏移 + 分配.大小;
			if (至少空间 > 空间)
			{
				空间 = 至少空间 * 2;
				分配器(空间, 头指针);
			}
			return reinterpret_cast<T*>(头指针 + 分配.偏移);
		}
		//返回的句柄可以长期使用，除非deallocate、管理器删除或底层资源被改动。
		template<typename T>
		内存句柄<T>allocate(size_type 个数 = 1)
		{
			size_type A;
			分配块* 分配块指针 = reinterpret_cast<分配块*>(分配头指针 + 1);
			for (A = 0; A < 分配头指针->分配块个数; ++A)
			{
				if (分配块指针->大小)
					分配块指针++;
				else
					break;
			}
			if (A == 分配头指针->分配块个数)
			{
				const size_type 至少块个数 = (A + 1) * 2;
				if (至少块个数 > 分配头指针->分配块个数)
				{
					const size_type 扩张空间 = (至少块个数 - 分配头指针->分配块个数) * sizeof(分配块);
					const size_type 数据段原起点 = sizeof(分配头) + sizeof(分配块) * 分配头指针->分配块空间;
					const size_type 数据段新起点 = 数据段原起点 + 扩张空间;

				}
			}
		}
		//释放后的句柄不再可用。
		template<typename T>
		void deallocate(内存句柄<T>句柄)const;
		//重分配不改变句柄本身的值，但可能会改变句柄指向的真实内存地址。
		template<typename T>
		void reallocate(内存句柄<T>句柄, size_type 个数);
	protected:
		struct 分配头
		{
			size_type 分配块空间;
			size_type 分配块个数;
		};
		union
		{
			char* 头指针;
			分配头* 分配头指针;
		};
		size_type 空间;
		struct 分配块
		{
			size_type 偏移;
			size_type 大小;
		};
	};
}