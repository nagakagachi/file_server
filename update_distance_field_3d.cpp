void UpdateDistanceField()
	{
		/*
			3次元の L1-DistanceTransform(DistanceField) を O(DN) で計算する.
			このアルゴリズムは各軸毎に一度実行することで最終的な結果が得られる.
			Pedro F. Felzenszwalb, Daniel P. Huttenlocher [Distance Transforms of Sampled Functions]
			https://prideout.net/blog/distance_fields/#the-marching-parabolas-algorithm
		*/
		auto L1AxisSignedDistanceTransform3d = []( auto& buffer3d, const int (&shape)[3], const int(&step)[3])
		{
			int axis_indices[3] = { 0,1,2 };
			int axis_count = 0;
			for (int i = 0; i < 3; ++i)
			{
				if (0 != step[i])
					axis_indices[(3 - 1)] = i;
				else
					axis_indices[axis_count++] = i;
			}

			// first step ( Order(N) )
			{
				int pos[3] = { 0,0,0 };
				for (int k = 0; k < shape[axis_indices[0]]; ++k)
				{
					pos[axis_indices[0]] = k;
					for (int j = 0; j < shape[axis_indices[1]]; ++j)
					{
						pos[axis_indices[1]] = j;

						pos[axis_indices[2]] = 0;
						int buffer_index_prev = (pos[0]) + (pos[1] * shape[0]) + (pos[2] * shape[0] * shape[1]);
						for (int i = 1; i < shape[axis_indices[2]]; ++i)
						{
							pos[axis_indices[2]] = i;
							int buffer_index = (pos[0]) + (pos[1] * shape[0]) + (pos[2] * shape[0] * shape[1]);

							if (0 < buffer3d[buffer_index])
							{
								// 外部の場合
								auto cn = (0 > buffer3d[buffer_index_prev]) ? 0 : buffer3d[buffer_index_prev];
								cn = cn + 1;
								buffer3d[buffer_index] = (buffer3d[buffer_index] < cn)? buffer3d[buffer_index] : cn;
							}
							else
							{
								// 内部の場合
								auto cn = (0 < buffer3d[buffer_index_prev]) ? 0 : buffer3d[buffer_index_prev];
								cn = -cn + 1;
								buffer3d[buffer_index] = -( (-buffer3d[buffer_index] < cn)? -buffer3d[buffer_index] : cn);
							}
							buffer_index_prev = buffer_index;
						}
					}
				}
			}
			// second step ( Order(N) )
			{
				int pos[3] = { 0,0,0 };
				for (int k = 0; k < shape[axis_indices[0]]; ++k)
				{
					pos[axis_indices[0]] = k;
					for (int j = 0; j < shape[axis_indices[1]]; ++j)
					{
						pos[axis_indices[1]] = j;

						pos[axis_indices[2]] = shape[axis_indices[2]] - 1;
						int buffer_index_prev = (pos[0]) + (pos[1] * shape[0]) + (pos[2] * shape[0] * shape[1]);
						for (int i = shape[axis_indices[2]] - 2; i >= 0; --i)
						{
							pos[axis_indices[2]] = i;
							int buffer_index = (pos[0]) + (pos[1] * shape[0]) + (pos[2] * shape[0] * shape[1]);

							if (0 < buffer3d[buffer_index])
							{
								// 外部の場合
								auto cn = (0 > buffer3d[buffer_index_prev]) ? 0 : buffer3d[buffer_index_prev];
								cn = cn + 1;
								buffer3d[buffer_index] = (buffer3d[buffer_index] < cn) ? buffer3d[buffer_index] : cn;
							}
							else
							{
								// 内部の場合
								auto cn = (0 < buffer3d[buffer_index_prev]) ? 0 : buffer3d[buffer_index_prev];
								cn = -cn + 1;
								buffer3d[buffer_index] = -((-buffer3d[buffer_index] < cn) ? -buffer3d[buffer_index] : cn);
							}
							buffer_index_prev = buffer_index;
						}
					}
				}
			}
		};

		const auto& src = cell_buffer_[BACK_BUFFER_INDEX];

		// SignedDistanceTransform初期値設定
		const auto max_distance = resolution_.X + resolution_.Y + resolution_.Z;
		for (int i = 0; i < src.Num(); ++i)
		{
			if (0 < src[i])
			{
				// 障害物は負の最大L1距離.
				distance_l1_[i] = -max_distance;
			}
			else
			{
				// 障害物ではない場合は正の最大L1距離.
				distance_l1_[i] = max_distance;
			}
		}

		const int shape[3] = { resolution_.X, resolution_.Y, resolution_.Z };
		const int step_x[3] = { 1, 0, 0 };
		const int step_y[3] = { 0, 1, 0 };
		const int step_z[3] = { 0, 0, 1 };
		// 3軸で実行.
		L1AxisSignedDistanceTransform3d(distance_l1_, shape, step_x);
		L1AxisSignedDistanceTransform3d(distance_l1_, shape, step_y);
		L1AxisSignedDistanceTransform3d(distance_l1_, shape, step_z);
	}
