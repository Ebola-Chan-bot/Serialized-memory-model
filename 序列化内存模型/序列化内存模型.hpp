import std;
namespace 序列化内存模型
{
	template<typename T>
	using 指针 = T*;
	struct 子分配器
	{
		//构造对象必须提供一个分配器，允许获取指定大小的span。如果分配大小大于原本大小，分配器必须保证原有范围内的原有数据不丢失。如果分配大小小于原本大小，分配器必须保证新范围内的原有数据不丢失。
		const std::move_only_function<std::span<char>(size_t 分配大小)const> 分配器;
		std::span<char>当前范围;
		操作视图(std::move_only_function<std::span<char>(size_t 分配大小)const> 分配器);

	protected:
		struct 分配块
		{
			size_t 偏移;
			size_t 大小;
		};
	};
}