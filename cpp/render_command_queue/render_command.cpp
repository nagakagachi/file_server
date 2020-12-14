
	/*
		Render Thread Command Queue.
			GameThreadからLambdaを登録してRenderThreadで実行するためのQueue.
	*/
	RenderCommandQueue::RenderCommandQueue()
	{
	}
	RenderCommandQueue::~RenderCommandQueue()
	{
		Finalize();
	}
	bool RenderCommandQueue::Initialize(const Desc& desc)
	{
		if (!desc.p_allocator)
			return false;
		
		desc_ = desc;
		// 念のため.
		desc_.page_size_in_byte = std::max(desc_.page_size_in_byte, u64(16));
		return true;
	}
	void RenderCommandQueue::Finalize()
	{
		queue_.begin() = queue_.end();
		for (auto&& e : command_page_buffer_)
		{
			if (e)
				delete e;
		}
	}
	void RenderCommandQueue::EnqueueInner(RenderCommand* cmd)
	{
		assert(desc_.p_allocator);
		queue_.push_back(cmd);
	}
	void* RenderCommandQueue::Alloc(u64 size)
	{
		// ページサイズより大きなコマンドは失敗.
		assert(size <= desc_.page_size_in_byte);
		if (size > desc_.page_size_in_byte)
		{
			return nullptr;
		}

		int page_id = 0;
		for (; page_id < command_buffer_used_.max_size() && (desc_.page_size_in_byte < (command_buffer_used_[page_id] + size)); ++page_id)
		{}

		if (command_buffer_used_.max_size() <= page_id)
		{
			// ページ拡張.

			const auto new_size = page_id + 1;

			DynamicArray<u64> new_buffer_used;
			DynamicArray<CommandBufferPage*> new_page_buffer;

			new_buffer_used.create(new_size, desc_.p_allocator);
			new_page_buffer.create(new_size, desc_.p_allocator);

			// copy
			new_buffer_used.fill(0);
			command_buffer_used_.copy_to(new_buffer_used);

			new_page_buffer.fill(nullptr);
			command_page_buffer_.copy_to(new_page_buffer);
			new_page_buffer[page_id] = NEW(*desc_.p_allocator) CommandBufferPage();
			new_page_buffer[page_id]->buffer.create(desc_.page_size_in_byte, desc_.p_allocator);

			// move
			command_buffer_used_ = std::move(new_buffer_used);
			command_page_buffer_ = std::move(new_page_buffer);
		}

		auto* mem = command_page_buffer_[page_id]->buffer.data() + command_buffer_used_[page_id];
		command_buffer_used_[page_id] += size;

		return mem;
	}

	RenderCommandList& RenderCommandQueue::GetQueue()
	{
		assert(desc_.p_allocator);
		return queue_;
	}
	void RenderCommandQueue::ClearQueue()
	{
		assert(desc_.p_allocator);
		// Queueをクリア. Bufferは維持.
		queue_.swap(RenderCommandList());

		command_buffer_used_.fill(0);
	}

	u64	RenderCommandQueue::GetCommandBufferSizeTotal() const
	{
		return command_page_buffer_.max_size() * desc_.page_size_in_byte;
	}
	u64	RenderCommandQueue::GetCommandBufferSizeUsed() const
	{
		u64 used = 0;
		for (auto&& e : command_buffer_used_)
			used += e;
		return used;
	}
