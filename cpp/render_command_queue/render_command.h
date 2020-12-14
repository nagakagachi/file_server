
	/*
		Render Thread Command.
	*/
	struct RenderCommand;
	typedef List<RenderCommand>	RenderCommandList;

	/*
		Render Thread Command Queue.
	*/
	struct RenderCommand
		: public RenderCommandList::Node
	{
		virtual void operator()() = 0;
	};

	/*
		Render Thread Command Template.
			Lambdaを格納する.
			Lambdaはキャプチャのみ, 引数無し.
			評価されるのは次のフレームのRenderThreadであるため,キャプチャに注意.
	*/
	template<typename LAMBDA>
	struct TRenderCommandType final : public RenderCommand
	{
	public:
		TRenderCommandType(LAMBDA&& InLambda) : Lambda(std::forward<LAMBDA>(InLambda)) {}

		// Call
		void operator()() final
		{
			Lambda();
		}
	private:
		LAMBDA Lambda;
	};

	/*
		Render Thread Command Queue.
			GameThreadからLambdaを登録してRenderThreadで実行するためのQueue.
	*/
	class RenderCommandQueue
	{
	public:

		// Lambdaをコマンドとして登録. キャプチャの寿命に注意.
		//	Enqueue([this]() {printf("%x\n", this); });
		template<typename LAMBDA>
		bool Enqueue(LAMBDA&& Lambda)
		{
			auto* mem = Alloc(sizeof(TRenderCommandType<LAMBDA>));
			if (!mem)
			{
				// Failed.
				return false;
			}
			// Inplace new.
			auto cmd = new(mem) TRenderCommandType<LAMBDA>(std::forward<LAMBDA>(Lambda));
			EnqueueInner(cmd);
			return true;
		}
		
		RenderCommandList& GetQueue();
		void ClearQueue();
		u64	GetCommandBufferSizeTotal() const;
		u64	GetCommandBufferSizeUsed() const;

	public:
		struct Desc
		{
			Allocator*	p_allocator = nullptr;
			u64					page_size_in_byte = 16 * 1024;;
		};

		RenderCommandQueue();
		~RenderCommandQueue();

		bool Initialize(const Desc& desc);
		void Finalize();

	private:
		void EnqueueInner(RenderCommand* cmd);
		void* Alloc(u64 size);

	private:
		Desc				desc_ = {};

		RenderCommandList	queue_;


		struct CommandBufferPage
		{
			DynamicArray<u8>	buffer;
		};

		DynamicArray<CommandBufferPage*>	command_page_buffer_ = {};
		DynamicArray<u64>				command_buffer_used_ = {};
	};
