#include<iostream>

using namespace std;
typedef void(*ALLOC_FUN)();//函数指针

//一级空间配置器(malloc,free,realloc)
template<int inst>//非类型模板参数
class MallocAllocTemplate
{
public:
	static void* Allocate(size_t n)
	{
		void* ret = malloc(n);
		if (0 == ret)
			ret = OomMalloc(n);
		return ret;
	}
	static void Deallocate(void* p)
	{
		free(p);
	}
	static void* Reallocate(void* p, size_t newsize)
	{
		void* ret = realloc(p, newsize);
		if (ret == 0)
			ret = OomRealloc(p, newsize);
		return ret;
	}
private:
	static void* OomMalloc(size_t n)//调用自定义的句柄处理函数释放并分配内存
	{
		ALLOC_FUN hander;
		void* ret;
		while (1)
		{
			hander = MallocAllocHander;
			if (0 == hander)
			{
				cout << "Out of memory" << endl;
				exit(-1);
			}
			hander();
			ret = malloc(n);
			if (ret)
			{
				rteurn (ret);
			}
		}
	}
	static void* OomRealloc(void* p, size_t newsize)
	{
		ALLOC_FUN hander;
		void* ret;
		while (1)
		{
			hander = MallocAllocHander;
			if (0 == hander)
			{
				cout << "Out of memory" << endl;
				exit(-1);
			}
			hander();
			ret = realloc(p,newsize);
			if (ret)
			{
				rteurn(ret);
			}
		}
	}
	static void(*SetMallocHandler(void(*f)()))();//设置操作系统分配内存失败时的句柄处理函数
	static ALLOC_FUN MallocAllocHander;

};
template<int inst>
ALLOC_FUN MallocAllocTemplate<inst>::MallocAllocHander = 0;//句柄函数初始化为0




//二级空间配置器
template<bool threads,int inst>
class DefaultAllocTemplate
{
private:
	enum{ ALIGN = 8 };
	enum{ MAX_BYTES = 128 };
	enum{ FREELISTSIZE = MAX_BYTES / ALIGN };
public:
	static void* Allocate(size_t n)
	{
		if (n > MAX_BYTES)
		{
			return MallocAllocTemplate<inst>::Allocate(n);
		}
		void* ret = NULL;
		size_t index = GetFreeListIndex(n);

		if (FreeList[index])//自由链表上有内存块
		{
			obj* cur = FreeList[index];
			ret = cur;
			FreeList[index] = cur->listLink;
		}
		else   //调用refill从内存池填充自由链表并返回内存池的第一个内存块
		{
			size_t bytes = GetRoundUpNum(n);
			return Refill(bytes);
		}
		return ret;
	}
	static void* Reallocate(void* p, size_t oldsize, size_t newsize)
	{
		void* ret = NULL;
		if (oldsize > (size_t)MAX_BYTES&&newsize > (size_t)MAX_BYTES)
			return (realloc(p, newsize));
		if (GetRoundUpNum(oldsize) == GetRoundUpNum(newsize))
			return p;
		ret = Allocate(newsize);
		size_t copysize = oldsize > newsize ? newsize : oldsize;
		memcopy(ret, p, copysize);
		DeAllocate(p, oldsize);
		return ret;
	}
	static void Deallocate(void* p, size_t n)
	{
		if (n > MAX_BYTES)//如果大于MAX_BYTES直接交还给一级空间配置器释放
			return MallocAllocTemplate<inst>::Deallocate(p, n);
		else//放回二级空间配置器的自由链表
		{
			size_t index = GetFreeListIndex(n);
			obj* tmp = (obj*)p;
			tmp->listLink = FreeList[index];
			Freelist[index] = tmp;
		}
	}
public:
	union obj
	{
		union obj* listLink;//自由链表中指向下一个内存快的指针
		char clientData[1];//调试用
	};
	static size_t GetFreeListIndex(size_t bytes)//得到所需内存块在自由链表中的下标
	{
		return ((bytes + ALIGN - 1) / ALIGN - 1);
	}
	static size_t GetRoundUpNum(size_t bytes)//得到内存块大小的向上对齐数
	{
		return (bytes + ALIGN - 1)&~(ALIGN - 1);
	}

	static void* Refill(size_t n)//从内存池拿出内存填充自由链表
	{
		int nobjs = 20;//申请20个n大小的内存块
		char* chunk = ChunkAlloc(n, nobjs);
		if (nobj == 1)//只分配到一个内存
		{
			return chunk;
		}
		obj* ret = NULL;
		obj* cur = NULL;
		size_t index = GetFreeListIndex(n);
		ret = (obj*)chunk;
		cur = (obj*)(chunk + n);

		//将nobj-2个内存块挂到自由链表上
		FreeList[index] = cur;
		for (int i = 2; i < nobjs; ++i)
		{
			cur->listLink = (obj*)(chunk + n*i);
			cur = cur->listLink;
		}
		cur->listLink = NULL;
		return ret;
	}
	static char* ChunkAlloc(size_t size, int& nobjs)
	{
		char* ret = NULL;
		size_t Leftbytes = endFree - startFree;
		size_t Needbytes = size * nobjs;
		if (Leftbytes >= Needbytes)
		{
			ret = startFree;
			startFree += Needbytes;
		}
		else if (Leftbytes >= size)//至少能分配到uoge内存块
		{
			ret = startFree;
			nobjs = Leftbytes / size;
			startFree += nobjs*size;
		}
		else     //一个内存块都分配不出来
		{
			if (Leftbytes > 0)
			{
				size_t index = GetFreeListIndex(Leftbytes);
				((obj*)startFree)->listLink = FreeList[index];
				FreeList[index] = (obj*)startFree;
				startFree = NULL;
			}
			//向操作系统申请2倍Needbytes加上已分配的heapsize/8的内存到内存池
			size_t getBytes = 2 * Needbytes + GetRoundUpNum(heapSize >> 4);
			startFree = (char*)malloc(getBytes);
			if (startFree == NULL)//从系统堆中分配内存失败
			{
				for (int i = size; i < MAX_BYTES; i += ALIGN)
				{
					obj* head = FreeList[GetFreeListIndex(i)];
					if (head)
					{
						startFree = (char*)head;
						head = head->listLink;
						endFree = startFree + i;
						return ChunkAlloc(size, nobjs);
					}
				}
				//最后的一根救命稻草，找一级空间配置器分配内存
				//（其他进程归还内存，调用自定义的句柄处理函数释放内存）
				startFree = MallocAllocTemplate<inst>::Allocate(getBytes);
			}
			heapSize += getBytes;//从系统堆分配的总字节数（可以用于下次分配时进行调节）
			endFree = startFree + getBytes;

			return ChunkAlloc(size, nobjs);//递归调用获取内存
		}
		return ret;
	}

	static obj* volatile FreeList[FREELISTSIZE];
	static char* startFree;
	static char* endFree;
	static size_t heapSize;

};


//typename表示DefaultAllocTemplate<threads, inst>是一个类型，
//如果不标识，编译器对此模板一无所知
template<bool threads, int inst>
typename DefaultAllocTemplate<threads, inst>::obj* volatile   
           DefaultAllocTemplate<threads, inst>::FreeList[FREELISTSIZE] = { 0 };

template<bool threads, int inst>
char* DefaultAllocTemplate<threads, inst>::startFree = 0;

template<bool threads, int inst>
char* DefaultAllocTemplate<threads, inst>::endFree = 0;

template<bool threads, int inst>
size_t DefaultAllocTemplate<threads, inst>::heapSize = 0;
