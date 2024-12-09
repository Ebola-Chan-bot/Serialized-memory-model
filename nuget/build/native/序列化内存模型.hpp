#pragma once
#ifdef __cpp_lib_modules
import std;
#else
#include <functional>
#include <queue>
#ifndef __cpp_lib_move_only_function
namespace std
{
	template <typename... T>
	using move_only_function = function<T...>;
}
#endif
#ifdef __cpp_lib_span
#include <span>
#endif
#endif
namespace 序列化内存模型
{
	template <typename ptrdiff_t = std::ptrdiff_t>
	struct 容器;
	// 通常不直接使用此对象，而是通过指针继承
	template <typename ptrdiff_t = std::ptrdiff_t>
	struct 句柄
	{
		using difference_type = ptrdiff_t;
		容器<ptrdiff_t> 容器引用;
		const ptrdiff_t 块索引;
		constexpr 句柄(容器<ptrdiff_t> &容器引用, ptrdiff_t 索引) : 容器引用(容器引用), 块索引(索引) {}
		// 释放后的句柄/指针不再可用。释放未分配或已释放的句柄是未定义行为。
		void deallocate() const;

	protected:
		void reallocate(ptrdiff_t 字节数) const;
		void *to_address() const;
	};
	// 专用于序列化内存模型容器的缀饰指针
	template <typename T, typename ptrdiff_t = std::ptrdiff_t>
	struct 指针 : 句柄<ptrdiff_t>
	{
		using element_type = T;
		constexpr 指针(容器<ptrdiff_t> &容器引用, ptrdiff_t 索引) : 句柄<ptrdiff_t>(容器引用, 索引) {}
		template <typename 任意类型>
		constexpr explicit 指针(const 指针<任意类型, ptrdiff_t> &源) : 句柄<ptrdiff_t>(源.容器引用, 源.索引) {}
		T *to_address() const
		{
			return reinterpret_cast<T *>(句柄<ptrdiff_t>::to_address());
		}
		T &operator*() const
		{
			return *to_address();
		}
		T *operator->() const
		{
			return to_address();
		}
		T &operator[](ptrdiff_t 索引) const
		{
			return to_address()[索引];
		}
		T *operator+(ptrdiff_t 索引) const
		{
			return to_address() + 索引;
		}
		ptrdiff_t operator ptrdiff_t() const
		{
			return 索引;
		}
		// 重分配不改变指针本身，但可能会改变指向的真实内存地址。也可以指定一个尚未分配的指针，将为这个指定的指针分配内存。
		void reallocate(ptrdiff_t 个数) const
		{
			句柄<ptrdiff_t>::reallocate(个数 * sizeof(T));
		}
	};
	// 可变指针的指针解引用会退化为索引。要避免退化，将模板参数改为const指针
	template <typename T, typename ptrdiff_t>
	struct 指针<指针<T, ptrdiff_t>, ptrdiff_t> : 句柄<ptrdiff_t>
	{
		using element_type = ptrdiff_t;
		constexpr 指针(容器<ptrdiff_t> &容器引用, ptrdiff_t 索引) : 句柄<ptrdiff_t>(容器引用, 索引) {}
		template <typename 任意类型>
		constexpr explicit 指针(const 指针<任意类型, ptrdiff_t> &源) : 句柄<ptrdiff_t>(源.容器引用, 源.索引) {}
		ptrdiff_t *to_address()
		{
			return reinterpret_cast<ptrdiff_t *>(句柄<ptrdiff_t>::to_address());
		}
		ptrdiff_t &operator*() const
		{
			return *to_address();
		}
		ptrdiff_t &operator[](ptrdiff_t 索引) const
		{
			return to_address()[索引];
		}
		ptrdiff_t *operator+(ptrdiff_t 索引) const
		{
			return to_address() + 索引;
		}
		ptrdiff_t operator ptrdiff_t() const
		{
			return 索引;
		}
		// 重分配不改变指针本身，但可能会改变指向的真实内存地址。也可以指定一个尚未分配的指针，将为这个指定的指针分配内存。
		void reallocate(ptrdiff_t 个数) const
		{
			句柄<ptrdiff_t>::reallocate(个数 * sizeof(ptrdiff_t));
		}
	};
	template <typename T, typename ptrdiff_t>
	struct 指针<const 指针<T, ptrdiff_t>, ptrdiff_t> : 句柄<ptrdiff_t>
	{
		using element_type = const 指针<T, ptrdiff_t>;
		using difference_type = ptrdiff_t;
		constexpr 指针(容器<ptrdiff_t> &容器引用, ptrdiff_t 索引) : 句柄<ptrdiff_t>(容器引用, 索引) {}
		template <typename 任意类型>
		constexpr explicit 指针(const 指针<任意类型, ptrdiff_t> &源) : 句柄<ptrdiff_t>(源.容器引用, 源.索引) {}
		// 指针不是以完整形式存在容器中，因此只能获取索引的指针
		const ptrdiff_t *to_address()
		{
			return reinterpret_cast<const ptrdiff_t *>(句柄<ptrdiff_t>::to_address());
		}
		// 返回的是临时构造的对象值，而非引用
		指针<T, ptrdiff_t> operator*() const
		{
			return 指针<T, ptrdiff_t>(容器引用, *to_address());
		}
		// 返回的是临时构造的对象值，而非引用
		指针<T, ptrdiff_t> &operator[](ptrdiff_t 索引) const
		{
			return 指针<T, ptrdiff_t>(容器引用, to_address()[索引]);
		}
		// 指针不是以完整形式存在容器中，因此只能获取索引的指针
		ptrdiff_t *operator+(ptrdiff_t 索引) const
		{
			return to_address() + 索引;
		}
		ptrdiff_t operator ptrdiff_t() const
		{
			return 索引;
		}
	};
	template <typename ptrdiff_t>
	struct 容器
	{
		const std::move_only_function<bool(ptrdiff_t &, char *&) const> 分配器;
		/*构造管理器需要用户提供一个底层字节序列特定的分配器。分配器可能被多次调用，应当保证不改变底层字节序列。分配器应当分配不少于输入空间参数值的字节数，并将实际分配的字节数写入空间参数，分配的内存指针写入头指针参数。最后返回一个bool，指示分配的空间是否曾经被管理器写入过：若true，则管理器将可以读取以前保存的内容；否则将覆盖创建新的内容。
		管理器将获得分配器的所有权，管理器删除时分配器也删除。用户应当在分配器的删除过程中负责释放分配的资源。
		*/
		容器(std::move_only_function<bool(ptrdiff_t &空间, char *&头指针) const> &&分配器) : 分配器(std::move(分配器)), 文件空间(sizeof(文件头))
		{
			if (!分配器(文件空间, 字节头指针))
				*文件头指针 = {0, 无效值};
			分配块头指针 = reinterpret_cast<分配块 *>(文件头指针 + 1);
		}
		// 返回的指针可以长期使用，除非deallocate、容器删除或底层资源被改动。
		template <typename T>
		指针<T, ptrdiff_t> allocate(ptrdiff_t 个数 = 1)
		{
			ptrdiff_t 新块索引;
			ptrdiff_t 至少空间 = sizeof(文件头) + sizeof(分配块);
			确保至少空间(至少空间);
			for (新块索引 = 0; 新块索引 < 文件头指针->分配块个数 && 分配块头指针[新块索引].偏移 != 无效值; 新块索引++)
				确保至少空间(至少空间 += sizeof(分配块));
			分配块头指针[新块索引] = 新块索引 < 文件头指针->分配块个数 ? 搜索空隙(sizeof(T) * 个数, 新块索引) : 分配块区扩张(新块索引, sizeof(T) * 个数);
			return 指针<T, ptrdiff_t>(*this, 新块索引);
		}

	protected:
		friend struct 句柄<ptrdiff_t>;
		static constexpr ptrdiff_t 无效值 = -1;
		struct 分配块
		{
			ptrdiff_t 上一块;
			ptrdiff_t 偏移;
			ptrdiff_t 大小;
			ptrdiff_t 下一块;
		};
		struct 文件头
		{
			ptrdiff_t 分配块个数; // 空块必须用无效值填充偏移字段
			ptrdiff_t 第一块;
		};
		union
		{
			char *字节头指针;
			文件头 *文件头指针;
		};
		分配块 *分配块头指针;
		ptrdiff_t 文件空间;
		void 确保至少空间(ptrdiff_t 至少空间)
		{
			if (至少空间 > 文件空间) [[unlikely]]
			{
				文件空间 = 至少空间 * 2;
				分配器(文件空间, 字节头指针);
				分配块头指针 = reinterpret_cast<分配块 *>(文件头指针 + 1);
			}
		}
		void 设置上下块(ptrdiff_t 上块索引, ptrdiff_t 本块索引, ptrdiff_t 下块索引) const
		{
			if (上块索引 == 无效值)
				文件头指针->第一块 = 本块索引;
			else
				分配块头指针[上块索引].下一块 = 本块索引;
			if (下块索引 != 无效值)
				分配块头指针[下块索引].上一块 = 本块索引;
		}
		分配块 分配块区扩张(ptrdiff_t 新块索引, ptrdiff_t 字节数)
		{
			const ptrdiff_t 原本分配块个数 = 文件头指针->分配块个数;
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
					ptrdiff_t 字节数 = std::min(分配块头指针[新块值.下一块].偏移 - 新块值.偏移, 缓冲区.size());
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
				for (ptrdiff_t i = 0; i < 下块引用.大小; i++)
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
		分配块 搜索空隙(ptrdiff_t 字节数, ptrdiff_t 块索引)
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
	template<typename ptrdiff_t>
	void *句柄<ptrdiff_t>::to_address() const
	{
		const ptrdiff_t 偏移 = sizeof(容器<ptrdiff_t>::文件头) + sizeof(容器<ptrdiff_t>::分配块) * 块索引;
		容器引用.确保至少空间(偏移 + sizeof(容器<ptrdiff_t>::分配块));
		const 容器<ptrdiff_t>::分配块 *分配块指针 = reinterpret_cast<容器<ptrdiff_t>::分配块 *>(容器引用.字节头指针 + 偏移);
		容器引用.确保至少空间(分配块指针->偏移 + 分配块指针->大小);
		return reinterpret_cast<void *>(容器引用.字节头指针 + 分配块指针->偏移);
	}
	template<typename ptrdiff_t>
	void 句柄<ptrdiff_t>::reallocate(ptrdiff_t 字节数) const
	{
		if (块索引 < 容器引用.文件头指针->分配块个数)
		{
			容器引用.确保至少空间(sizeof(容器<ptrdiff_t>::文件头) + sizeof(容器<ptrdiff_t>::分配块) * (块索引 + 1));
			容器<ptrdiff_t>::分配块 块值 = 容器引用.分配块头指针[块索引]; // 无法维持指针有效且所有字段都被用到，不如直接拷贝
			块值.大小 = 字节数;
			if (块值.偏移 == 容器<ptrdiff_t>::无效值)
				块值 = 容器引用.搜索空隙(块值.大小, 块索引);
			else if (块值.下一块 != 容器<ptrdiff_t>::无效值)
			{
				容器引用.确保至少空间(sizeof(容器<ptrdiff_t>::文件头) + sizeof(容器<ptrdiff_t>::分配块) * (块值.下一块 + 1));
				if (容器引用.分配块头指针[块值.下一块].偏移 < 块值.偏移 + 块值.大小)
				{
					容器引用.分配块头指针[块值.下一块].上一块 = 块值.上一块;
					if (块值.上一块 == 容器<ptrdiff_t>::无效值)
						容器引用.文件头指针->第一块 = 块值.下一块;
					else
					{
						容器引用.确保至少空间(sizeof(容器<ptrdiff_t>::文件头) + sizeof(容器<ptrdiff_t>::分配块) * (块值.上一块 + 1));
						容器引用.分配块头指针[块值.上一块].下一块 = 块值.下一块;
					}
					块值 = 容器引用.搜索空隙(块值.大小, 块索引);
				}
			}
			容器引用.分配块头指针[块索引] = 块值;
		}
		else
			容器引用.分配块头指针[块索引] = 容器引用.分配块区扩张(块索引, 字节数);
	}
	template<typename ptrdiff_t>
	void 句柄<ptrdiff_t>::deallocate() const
	{
		容器引用.确保至少空间(sizeof(容器<ptrdiff_t>::文件头) + sizeof(容器<ptrdiff_t>::分配块) * (块索引 + 1));
		容器<ptrdiff_t>::分配块 &旧块引用 = 容器引用.分配块头指针[块索引];
		旧块引用.偏移 = 容器<ptrdiff_t>::无效值;
		const ptrdiff_t 上块索引 = 旧块引用.上一块;
		const ptrdiff_t 下块索引 = 旧块引用.下一块;
		if (上块索引 == 容器<ptrdiff_t>::无效值)
			容器引用.文件头指针->第一块 = 下块索引;
		else
		{
			容器引用.确保至少空间(sizeof(容器<ptrdiff_t>::文件头) + sizeof(容器<ptrdiff_t>::分配块) * (上块索引 + 1));
			容器引用.分配块头指针[上块索引].下一块 = 下块索引;
		}
		if (下块索引 != 容器<ptrdiff_t>::无效值)
		{
			容器引用.确保至少空间(sizeof(容器<ptrdiff_t>::文件头) + sizeof(容器<ptrdiff_t>::分配块) * (下块索引 + 1));
			容器引用.分配块头指针[下块索引].上一块 = 上块索引;
		}
	}
}