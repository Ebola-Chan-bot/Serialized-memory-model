#pragma once
#ifdef __cpp_lib_modules
import std;
#else
#include <functional>
#include <queue>
#ifdef __cpp_lib_span
#include <span>
#endif
#endif
#ifndef __cpp_lib_move_only_function
namespace std
{
	template <typename T>
	using move_only_function = function<T>;
}
#endif
namespace 序列化内存模型
{
	// 内存句柄实现上是指针的别名，因此可以隐式转换为通用类型的void*或内存句柄<void>。但它不直接指向有效的内存，不应将其当作指针使用。始终通过管理器操作这些句柄。
	template <typename T>
	using 内存句柄 = T *;
	// 可选指定一个无符号整数类型作为内存管理器的索引类型，默认为size_t。如果你预期所需空间大小不会太大，可以使用较小的类型以节省内存。指定比size_t更大的类型通常没有意义。
	template <typename size_type = size_t>
	struct 管理器
	{
		const std::move_only_function<bool(size_type &, char *&) const> 分配器;
		/*构造管理器需要用户提供一个底层字节序列特定的分配器。分配器可能被多次调用，应当保证不改变底层字节序列。分配器应当分配不少于输入空间参数值的字节数，并将实际分配的字节数写入空间参数，分配的内存指针写入头指针参数。最后返回一个bool，指示分配的空间是否曾经被管理器写入过：若true，则管理器将可以读取以前保存的内容；否则将覆盖创建新的内容。
		管理器将获得分配器的所有权，管理器删除时分配器也删除。用户应当在分配器的删除过程中负责释放分配的资源。
		*/
		管理器(std::move_only_function<bool(size_type &空间, char *&头指针) const> &&分配器) : 分配器(std::move(分配器)), 文件空间(sizeof(文件头))
		{
			if (!分配器(文件空间, 字节头指针))
				*文件头指针 = {0, 无效值};
			分配块头指针 = reinterpret_cast<分配块 *>(文件头指针 + 1);
		}
		// 返回的引用只能临时使用。对管理器的任何操作都可能导致引用失效。使用未分配的句柄是未定义行为。
		template <typename T>
		T &operator*(内存句柄<T> 句柄)
		{
			return *to_address(句柄);
		}
		// 返回的指针只能临时使用。对管理器的任何操作都可能导致指针失效。使用未分配的句柄是未定义行为。
		template <typename T>
		T *to_address(内存句柄<T> 句柄)
		{
			const size_type 偏移 = sizeof(文件头) + sizeof(分配块) * reinterpret_cast<size_type>(句柄);
			确保至少空间(偏移 + sizeof(分配块));
			const 分配块 *分配块指针 = reinterpret_cast<分配块 *>(字节头指针 + 偏移);
			确保至少空间(分配块指针->偏移 + 分配块指针->大小);
			return reinterpret_cast<T *>(字节头指针 + 分配块指针->偏移);
		}
		// 返回的句柄可以长期使用，除非deallocate、管理器删除或底层资源被改动。
		template <typename T>
		内存句柄<T> allocate(size_type 个数 = 1)
		{
			size_type 新块索引;
			size_type 至少空间 = sizeof(文件头) + sizeof(分配块);
			确保至少空间(至少空间);
			for (新块索引 = 0; 新块索引 < 文件头指针->分配块个数 && 分配块头指针[新块索引].偏移 != 无效值; 新块索引++)
				确保至少空间(至少空间 += sizeof(分配块));
			分配块头指针[新块索引] = 新块索引 < 文件头指针->分配块个数 ? 搜索空隙(sizeof(T) * 个数, 新块索引) : 分配块区扩张(新块索引, sizeof(T) * 个数);
			return reinterpret_cast<内存句柄<T>>(新块索引);
		}
		// 释放后的句柄不再可用。释放未分配的句柄是未定义行为。
		void deallocate(内存句柄<void> 句柄)
		{
			const size_type 旧块索引 = reinterpret_cast<size_type>(句柄);
			确保至少空间(sizeof(文件头) + sizeof(分配块) * (旧块索引 + 1));
			分配块 &旧块引用 = 分配块头指针[旧块索引];
			旧块引用.偏移 = 无效值;
			const size_type 上块索引 = 旧块引用.上一块;
			const size_type 下块索引 = 旧块引用.下一块;
			if (上块索引 == 无效值)
				文件头指针->第一块 = 下块索引;
			else
			{
				确保至少空间(sizeof(文件头) + sizeof(分配块) * (上块索引 + 1));
				分配块头指针[上块索引].下一块 = 下块索引;
			}
			if (下块索引 != 无效值)
			{
				确保至少空间(sizeof(文件头) + sizeof(分配块) * (下块索引 + 1));
				分配块头指针[下块索引].上一块 = 上块索引;
			}
		}
		// 重分配不改变句柄本身的值，但可能会改变句柄指向的真实内存地址。也可以指定一个尚未分配的句柄，将为这个指定的句柄分配内存。
		template <typename T>
		void reallocate(内存句柄<T> 句柄, size_type 个数)
		{
			const size_type 块索引 = reinterpret_cast<size_type>(句柄);
			if (块索引 < 文件头指针->分配块个数)
			{
				确保至少空间(sizeof(文件头) + sizeof(分配块) * (块索引 + 1));
				分配块 块值 = 分配块头指针[块索引]; // 无法维持指针有效且所有字段都被用到，不如直接拷贝
				块值.大小 = sizeof(T) * 个数;
				if (块值.偏移 == 无效值)
					块值 = 搜索空隙(块值.大小, 块索引);
				else if (块值.下一块 != 无效值)
				{
					确保至少空间(sizeof(文件头) + sizeof(分配块) * (块值.下一块 + 1));
					if (分配块头指针[块值.下一块].偏移 < 块值.偏移 + 块值.大小)
					{
						分配块头指针[块值.下一块].上一块 = 块值.上一块;
						if (块值.上一块 == 无效值)
							文件头指针->第一块 = 块值.下一块;
						else
						{
							确保至少空间(sizeof(文件头) + sizeof(分配块) * (块值.上一块 + 1));
							分配块头指针[块值.上一块].下一块 = 块值.下一块;
						}
						块值 = 搜索空隙(块值.大小, 块索引);
					}
				}
				分配块头指针[块索引] = 块值;
			}
			else
				分配块头指针[块索引] = 分配块区扩张(块索引, sizeof(T) * 个数);
		}

	protected:
		static constexpr size_type 无效值 = -1;
		struct 分配块
		{
			size_type 上一块;
			size_type 偏移;
			size_type 大小;
			size_type 下一块;
		};
		struct 文件头
		{
			size_type 分配块个数; // 空块必须用无效值填充偏移字段
			size_type 第一块;
		};
		union
		{
			char *字节头指针;
			文件头 *文件头指针;
		};
		分配块 *分配块头指针;
		size_type 文件空间;
		void 确保至少空间(size_type 至少空间)
		{
			if (至少空间 > 文件空间) [[unlikely]]
			{
				文件空间 = 至少空间 * 2;
				分配器(文件空间, 字节头指针);
				分配块头指针 = reinterpret_cast<分配块 *>(文件头指针 + 1);
			}
		}
		void 设置上下块(size_type 上块索引, size_type 本块索引, size_type 下块索引) const
		{
			if (上块索引 == 无效值)
				文件头指针->第一块 = 本块索引;
			else
				分配块头指针[上块索引].下一块 = 本块索引;
			if (下块索引 != 无效值)
				分配块头指针[下块索引].上一块 = 本块索引;
		}
		分配块 分配块区扩张(size_type 新块索引, size_type 字节数)
		{
			const size_type 原本分配块个数 = 文件头指针->分配块个数;
			文件头指针->分配块个数 = (新块索引 + 1) * 2;
			分配块 新块值{无效值, sizeof(文件头) + sizeof(分配块) * 文件头指针->分配块个数, 字节数, 文件头指针->第一块};
			// 新扩展的块区都要填充无效值，因此可以预分配
			确保至少空间(sizeof(文件头) + sizeof(分配块) * 文件头指针->分配块个数);
			// 原有数据块现在可能占用了新的分配块位置，需要向后挪
			std::queue<char> 缓冲区;
			while (新块值.下一块 != 无效值)
			{
				if (分配块头指针[新块值.下一块].偏移 >= 新块值.偏移) [[unlikely]]
				{
					// 写出头后有空余空间可写，先尝试清缓存
					size_type 字节数 = std::min(分配块头指针[新块值.下一块].偏移 - 新块值.偏移, 缓冲区.size());
					确保至少空间(新块值.偏移 + 字节数);
					char *写出头 = 字节头指针 + 新块值.偏移;
					新块值.偏移 += 字节数;
					for (; 字节数; --字节数)
					{
						*写出头++ = 缓冲区.front();
						缓冲区.pop();
					}
					if (缓冲区.empty())
						// 如果缓冲区已清空，说明后续数据块无需再挪动
						break;
				}
				// 当前块数据无法直接写出，需要缓存。上个if块中可能发生过重分配，必须重新取得块指针。
				确保至少空间(分配块头指针[新块值.下一块].偏移 + 分配块头指针[新块值.下一块].大小);
				分配块 &下块引用 = 分配块头指针[新块值.下一块];
				下块引用.偏移 = 新块值.偏移 + 缓冲区.size();
#ifdef __cpp_lib_span
				缓冲区.push_range(std::span<const char>(字节头指针 + 下块引用.偏移, 下块引用.大小));
#else
				const char *读入头 = 字节头指针 + 下块引用.偏移;
				for (size_type i = 0; i < 下块引用.大小; i++)
					缓冲区.push(*读入头++);
#endif
				新块值.上一块 = 新块值.下一块; // 保存上块索引，分配新块时将要用到
				新块值.下一块 = 下块引用.下一块;
			}
			// 将缓冲区剩余数据写出
			if (缓冲区.size())
			{
				确保至少空间(新块值.偏移 + 缓冲区.size());
				char *写出头 = 字节头指针 + 新块值.偏移;
				新块值.偏移 += 缓冲区.size();
				while (缓冲区.size())
				{
					*写出头++ = 缓冲区.front();
					缓冲区.pop();
				}
			}
			// 确保上块和下块之间有足够的空间分配给新块
			while (新块值.下一块 != 无效值)
			{
				const 分配块 &下块引用 = 分配块头指针[新块值.下一块];
				if (下块引用.偏移 >= 新块值.大小 + 新块值.偏移)
					break;
				新块值.偏移 = 下块引用.偏移 + 下块引用.大小;
				新块值.上一块 = 新块值.下一块;
				新块值.下一块 = 下块引用.下一块;
			}
			// 为新块后的空块填充无效值
			std::fill(分配块头指针 + 原本分配块个数, 分配块头指针 + 文件头指针->分配块个数, 分配块{无效值, 无效值, 无效值, 无效值});
			设置上下块(新块值.上一块, 新块索引, 新块值.下一块);
			return 新块值;
		}
		分配块 搜索空隙(size_type 字节数, size_type 块索引)
		{
			分配块 新块值{无效值, sizeof(文件头) + sizeof(分配块) * 文件头指针->分配块个数, 字节数, 文件头指针->第一块};
			do
			{
				确保至少空间(sizeof(文件头) + sizeof(分配块) * (新块值.下一块 + 1));
				const 分配块 &下块引用 = 分配块头指针[新块值.下一块];
				if (下块引用.偏移 >= 新块值.偏移 + 新块值.大小)
					break;
				新块值.偏移 = 下块引用.偏移 + 下块引用.大小;
				新块值.上一块 = 新块值.下一块;
				新块值.下一块 = 下块引用.下一块;
			} while (新块值.下一块 != 无效值);
			设置上下块(新块值.上一块, 块索引, 新块值.下一块);
			return 新块值;
		}
	};
	// template <typename value_type,typename size_type=size_t>
	using value_type = int;
	using size_type = size_t;
	struct dynarray
	{
		const 内存句柄<value_type> 句柄;
		管理器<size_type>& 管理器引用;
		dynarray(管理器<size_type>& 管理器引用, size_type 个数) : 管理器引用(管理器引用), 句柄(管理器引用.allocate<value_type>(个数)) {}
	};
	template<typename T>
	struct 文件指针
	{
		
	};
}