/*
 * Copyright (c) 2017-2018 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __ARM_COMPUTE_MISC_SHAPE_CALCULATOR_H__
#define __ARM_COMPUTE_MISC_SHAPE_CALCULATOR_H__

#include "arm_compute/core/Helpers.h"
#include "arm_compute/core/ITensorInfo.h"
#include "arm_compute/core/Utils.h"

#include <cmath>

namespace arm_compute
{
namespace misc
{
namespace shape_calculator
{
inline TensorShape compute_vector_to_tensor_output_shape(const TensorShape &input, size_t conv_w, size_t conv_h, const DataLayout &data_layout)
{
    const size_t idx_w = get_data_layout_dimension_index(data_layout, DataLayoutDimension::WIDTH);
    const size_t idx_h = get_data_layout_dimension_index(data_layout, DataLayoutDimension::HEIGHT);
    const size_t idx_c = get_data_layout_dimension_index(data_layout, DataLayoutDimension::CHANNEL);

    TensorShape output_shape(input);
    output_shape.set(idx_w, conv_w);
    output_shape.set(idx_h, conv_h);
    output_shape.set(idx_c, input.x() / (conv_w * conv_h));

    return output_shape;
}
inline TensorShape compute_permutation_output_shape(const ITensorInfo &input, const PermutationVector &perm)
{
    TensorShape output_shape = input.tensor_shape();
    permute(output_shape, perm);
    return output_shape;
}
inline TensorShape compute_weights_reshaped_shape(const ITensorInfo &weights, bool has_bias = false, unsigned int num_groups = 1)
{
    // Number of groups greater than one are only supported for NCHW data layout, and the number of weights must be a multiple of it.
    ARM_COMPUTE_ERROR_ON(num_groups == 0);
    ARM_COMPUTE_ERROR_ON(weights.data_layout() == DataLayout::NHWC && num_groups > 1);
    ARM_COMPUTE_ERROR_ON((weights.dimension(3) % num_groups) != 0);

    // Calculate output shape
    TensorShape weights_reshaped{ weights.tensor_shape() };
    weights_reshaped.set(3, weights_reshaped[3] / num_groups);

    weights_reshaped.collapse(3);
    const size_t tmp_dim = weights_reshaped[0];
    weights_reshaped.set(0, weights_reshaped[1]);
    weights_reshaped.set(1, tmp_dim + (has_bias ? 1 : 0));
    if(weights.num_dimensions() < 5)
    {
        weights_reshaped.set(2, num_groups);
    }

    return weights_reshaped;
}
inline TensorShape compute_interleaved_shape(const ITensorInfo &a, int mult_interleave4x4_height = 1, bool reinterpret_input_as_3d = false)
{
    // The interleaved output matrix will have the following shape: [ a_height * W, ceil(a_width / W) ] where W = 4 * mult_interleave4x4_height
    ARM_COMPUTE_ERROR_ON(mult_interleave4x4_height < 1);
    const int   interleave_width = 4 * mult_interleave4x4_height;
    TensorShape shape_interleaved_a{ a.tensor_shape() };
    shape_interleaved_a.set(0, a.dimension(0) * interleave_width);
    if(reinterpret_input_as_3d)
    {
        const int M      = a.dimension(1) * a.dimension(2);
        const int height = std::ceil(M / static_cast<float>(interleave_width));
        shape_interleaved_a.set(1, height);
        shape_interleaved_a.remove_dimension(2);
    }
    else
    {
        shape_interleaved_a.set(1, std::ceil(a.dimension(1) / static_cast<float>(interleave_width)));
    }

    return shape_interleaved_a;
}
inline TensorShape compute_transpose1xW_shape(const ITensorInfo &b)
{
    // The transpose1xW output matrix will have the following shape: [ b_height * 16, ceil(b_width / 16.0f) ]
    TensorShape shape_transposed1xW_b{ b.tensor_shape() };
    shape_transposed1xW_b.set(0, b.dimension(1) * 16);
    shape_transposed1xW_b.set(1, std::ceil(b.dimension(0) / 16.f));

    return shape_transposed1xW_b;
}
inline TensorShape compute_transpose1xW_with_element_size_shape(const ITensorInfo &b, int mult_transpose1xW_width = 1)
{
    // Note: mult_transpose1xW_width expresses the number of chunks with size 1x(W) we want to store on the same row
    //       The transpose1xW output matrix will have the following shape:
    //       [ b_height * W, ceil(b_width / W) ] where W = (16 / element size of the tensor) * mult_transpose1xW_width
    ARM_COMPUTE_ERROR_ON(mult_transpose1xW_width < 1);
    TensorShape  shape_transposed1xW_b{ b.tensor_shape() };
    const size_t transpose_width = (16 / b.element_size()) * mult_transpose1xW_width;
    shape_transposed1xW_b.set(0, b.dimension(1) * transpose_width);
    shape_transposed1xW_b.set(1, static_cast<size_t>(std::ceil(b.dimension(0) / static_cast<float>(transpose_width))));

    return shape_transposed1xW_b;
}
inline TensorShape compute_reductionA_shape(const ITensorInfo &b)
{
    TensorShape shape_vector_sum_col{ b.tensor_shape() };
    if(shape_vector_sum_col.num_dimensions() > 1)
    {
        shape_vector_sum_col.remove_dimension(1);
    }

    return shape_vector_sum_col;
}
inline TensorShape compute_reductionB_shape(const ITensorInfo &a)
{
    TensorShape shape_vector_sum_row{ a.tensor_shape() };
    shape_vector_sum_row.set(Window::DimX, a.dimension(1));
    if(a.num_dimensions() > 1)
    {
        shape_vector_sum_row.remove_dimension(1);
    }

    return shape_vector_sum_row;
}
inline TensorShape compute_col2im_shape(const ITensorInfo &input, std::pair<unsigned int, unsigned int> convolved_dims, unsigned int num_groups = 1)
{
    ARM_COMPUTE_ERROR_ON(num_groups == 0);
    ARM_COMPUTE_ERROR_ON(input.tensor_shape()[1] != (convolved_dims.first * convolved_dims.second));
    ARM_COMPUTE_ERROR_ON((num_groups > 1) && input.tensor_shape()[2] != num_groups);

    TensorShape col2im_shape{ input.tensor_shape() };
    col2im_shape.set(0, convolved_dims.first);
    col2im_shape.set(1, convolved_dims.second);
    col2im_shape.set(2, input.tensor_shape()[0] * num_groups);

    const unsigned int batch_idx = (num_groups == 1) ? 2 : 3;
    col2im_shape.set(3, input.tensor_shape()[batch_idx]);

    return col2im_shape;
}
inline TensorShape compute_transposed_shape(const ITensorInfo &input)
{
    TensorShape shape_transposed{ input.tensor_shape() };

    shape_transposed.set(0, input.dimension(1));
    shape_transposed.set(1, input.dimension(0));

    return shape_transposed;
}
inline TensorShape compute_depthwise_convolution_shape(const ITensorInfo &input, const ITensorInfo &weights, PadStrideInfo conv_info, unsigned int depth_multiplier)
{
    const TensorShape input_shape{ input.tensor_shape() };
    const TensorShape weights_shape{ weights.tensor_shape() };

    const DataLayout data_layout = input.data_layout();
    const int        width_idx   = get_data_layout_dimension_index(data_layout, DataLayoutDimension::WIDTH);
    const int        height_idx  = get_data_layout_dimension_index(data_layout, DataLayoutDimension::HEIGHT);
    const int        channel_idx = get_data_layout_dimension_index(data_layout, DataLayoutDimension::CHANNEL);

    unsigned int output_width  = 0;
    unsigned int output_height = 0;
    std::tie(output_width, output_height) = scaled_dimensions(input_shape[width_idx], input_shape[height_idx],
                                                              weights_shape[width_idx], weights_shape[height_idx],
                                                              conv_info);

    TensorShape output_shape{ input_shape };
    output_shape.set(width_idx, output_width);
    output_shape.set(height_idx, output_height);
    output_shape.set(channel_idx, input_shape[channel_idx] * depth_multiplier);

    return output_shape;
}
inline TensorShape compute_deconvolution_shape(const ITensorInfo &input, unsigned int sx, unsigned int sy, unsigned int inner_border_right, unsigned int inner_border_top, const PadStrideInfo &info)
{
    TensorShape        scale_out_shape(input.tensor_shape());
    const unsigned int out_x = input.dimension(0) + (input.dimension(0) - 1) * (sx - 1) + inner_border_right + 2 * info.pad().first;
    const unsigned int out_y = input.dimension(1) + (input.dimension(1) - 1) * (sy - 1) + inner_border_top + 2 * info.pad().second;
    scale_out_shape.set(0, out_x);
    scale_out_shape.set(1, out_y);

    return scale_out_shape;
}
inline TensorShape compute_im2col_conv_shape(const ITensorInfo *input, const Size2D &kernel_dims, const PadStrideInfo &conv_info, bool has_bias, const Size2D &dilation, bool batch_size_on_z,
                                             unsigned int num_groups = 1)
{
    // The output shape will be the 3D shape [ out_channels * kernel_area, num_elems_per_out_channel, batches ]                           if batch_size_on_z == true
    //                       or the 4D shape [ out_channels * kernel_area / num_groups, num_elems_per_out_channel, num_groups, batches ]  if batch_size_on_z == false

    ARM_COMPUTE_ERROR_ON(num_groups == 0);
    ARM_COMPUTE_ERROR_ON(num_groups > 1 && input->data_layout() != DataLayout::NCHW);
    ARM_COMPUTE_ERROR_ON(num_groups > 1 && batch_size_on_z);

    TensorShape output_shape{ input->tensor_shape() };

    const DataLayout data_layout = input->data_layout();
    const int        width_idx   = get_data_layout_dimension_index(data_layout, DataLayoutDimension::WIDTH);
    const int        height_idx  = get_data_layout_dimension_index(data_layout, DataLayoutDimension::HEIGHT);
    const int        channel_idx = get_data_layout_dimension_index(data_layout, DataLayoutDimension::CHANNEL);

    std::pair<unsigned int, unsigned int> out_dims = scaled_dimensions(output_shape[width_idx], output_shape[height_idx], kernel_dims.width, kernel_dims.height, conv_info, dilation);
    output_shape.set(0, (output_shape[channel_idx] / num_groups * kernel_dims.area() + (has_bias ? 1 : 0))); // NOLINT
    output_shape.set(1, (out_dims.first * out_dims.second));
    if(batch_size_on_z && output_shape.num_dimensions() >= 3)
    {
        output_shape.remove_dimension(2);
    }
    else
    {
        output_shape.set(2, num_groups);
    }

    return output_shape;
}
inline TensorShape compute_flatten_shape(const ITensorInfo *input)
{
    // The output shape will be the flatten version of the input (i.e. [ width * height * channels, num_batches, ... ] ). Used for FlattenLayer and FullyConnectedLayer.

    TensorShape output_shape{ input->tensor_shape() };

    output_shape.collapse(3);

    return output_shape;
}
inline TensorShape compute_interleave_custom_shape(const TensorShape &input, const int x_interleave, const int y_interleave)
{
    TensorShape output_shape{ input };

    output_shape.set(0, output_shape.x() * x_interleave);
    output_shape.set(1, std::ceil(output_shape.y() / static_cast<float>(y_interleave)));

    return output_shape;
}
inline TensorShape compute_fully_connected_reshaped_weights_shape(const ITensorInfo *input, bool transpose_weights, bool is_batched_fc_layer, const int interleave)
{
    TensorShape output_shape{ input->tensor_shape() };

    // Transpose weights if the user hasn't done it
    if(transpose_weights)
    {
        output_shape = compute_transposed_shape(*input);
    }

    // If we run multiple batches we need 1xW transpose, too.
    if(is_batched_fc_layer)
    {
        output_shape = compute_transposed_shape(input->clone()->set_tensor_shape(output_shape));
        output_shape = compute_interleave_custom_shape(output_shape, interleave, interleave);
    }

    return output_shape;
}

inline TensorShape compute_winograd_filter_transform_shape(const ITensorInfo &input, const WinogradInfo &winograd_info)
{
    TensorShape tensor_shape{ input.tensor_shape() };

    const Size2D kernel_size      = winograd_info.kernel_size;
    const Size2D output_tile_size = winograd_info.output_tile_size;
    const Size2D input_tile_size  = Size2D(output_tile_size.width + kernel_size.width - 1, output_tile_size.height + kernel_size.height - 1);

    tensor_shape.remove_dimension(get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::WIDTH));
    tensor_shape.set(Window::DimX, input.dimension(3));
    tensor_shape.set(Window::DimY, input.dimension(get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::CHANNEL)));
    tensor_shape.set(Window::DimZ, input_tile_size.area());

    return tensor_shape;
}
inline TensorShape compute_winograd_input_transform_shape(const ITensorInfo &input, const WinogradInfo &winograd_info)
{
    const PadStrideInfo conv_info        = winograd_info.convolution_info;
    const Size2D        kernel_size      = winograd_info.kernel_size;
    const Size2D        output_tile_size = winograd_info.output_tile_size;
    const Size2D        input_tile_size  = Size2D(output_tile_size.width + kernel_size.width - 1, output_tile_size.height + kernel_size.height - 1);

    const size_t idx_w = get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::WIDTH);
    const size_t idx_h = get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::HEIGHT);
    const size_t idx_c = get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::CHANNEL);

    // Compute the number of output tiles along the x and y direction of size "output_tile_size"
    const Size2D num_tiles = compute_winograd_convolution_tiles(Size2D(input.tensor_shape()[idx_w], input.tensor_shape()[idx_h]),
                                                                kernel_size,
                                                                output_tile_size,
                                                                conv_info);

    const unsigned int width  = input.tensor_shape()[idx_c];
    const unsigned int height = num_tiles.area();
    const unsigned int depth  = input_tile_size.area();

    TensorShape output_shape{ input.tensor_shape() };
    output_shape.set(0, width);
    output_shape.set(1, height);
    output_shape.set(2, depth);

    return output_shape;
}
inline TensorShape compute_winograd_output_transform_shape(const ITensorInfo &input, const WinogradInfo &winograd_info)
{
    const PadStrideInfo conv_info        = winograd_info.convolution_info;
    const Size2D        kernel_size      = winograd_info.kernel_size;
    const Size2D        input_dimensions = winograd_info.input_dimensions;
    const DataLayout    data_layout      = winograd_info.output_data_layout;

    // Compute output shape
    unsigned int output_width  = 0;
    unsigned int output_height = 0;
    std::tie(output_width, output_height) = scaled_dimensions(input_dimensions.width, input_dimensions.height,
                                                              kernel_size.width, kernel_size.height, conv_info);

    TensorShape tensor_shape{ input.tensor_shape() };

    // Output dimension
    const unsigned int out_w = output_width;
    const unsigned int out_h = output_height;
    const unsigned int out_c = input.dimension(0);

    tensor_shape.set(get_data_layout_dimension_index(data_layout, DataLayoutDimension::WIDTH), out_w);
    tensor_shape.set(get_data_layout_dimension_index(data_layout, DataLayoutDimension::HEIGHT), out_h);
    tensor_shape.set(get_data_layout_dimension_index(data_layout, DataLayoutDimension::CHANNEL), out_c);

    return tensor_shape;
}
inline TensorShape compute_deep_convolution_shape(const ITensorInfo &input, const ITensorInfo &weights, PadStrideInfo conv_info)
{
    const TensorShape input_shape{ input.tensor_shape() };
    const TensorShape weights_shape{ weights.tensor_shape() };

    const size_t idx_width   = get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::WIDTH);
    const size_t idx_height  = get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::HEIGHT);
    const size_t idx_channel = get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::CHANNEL);

    const unsigned int input_width         = input_shape[idx_width];
    const unsigned int input_height        = input_shape[idx_height];
    const unsigned int weights_width       = weights_shape[idx_width];
    const unsigned int weights_height      = weights_shape[idx_height];
    const unsigned int weights_out_channel = weights_shape[3];
    unsigned int       output_width        = 0;
    unsigned int       output_height       = 0;
    std::tie(output_width, output_height) = scaled_dimensions(input_width, input_height, weights_width, weights_height, conv_info);

    TensorShape output_shape{ input_shape };
    output_shape.set(idx_width, output_width);
    output_shape.set(idx_height, output_height);
    output_shape.set(idx_channel, weights_out_channel);

    return output_shape;
}
inline TensorShape compute_min_max_shape(const ITensorInfo *input)
{
    TensorShape output_shape{ input->tensor_shape() };
    output_shape.set(Window::DimX, 2);
    output_shape.remove_dimension(1);
    output_shape.remove_dimension(1);

    return output_shape;
}

inline TensorShape compute_pool_shape(const ITensorInfo &input, PoolingLayerInfo pool_info)
{
    unsigned int pooled_w = 0;
    unsigned int pooled_h = 0;

    TensorShape output_shape{ input.tensor_shape() };

    const bool         is_global_pooling = pool_info.is_global_pooling();
    const unsigned int idx_width         = get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::WIDTH);
    const unsigned int idx_height        = get_data_layout_dimension_index(input.data_layout(), DataLayoutDimension::HEIGHT);
    const unsigned int pool_size_x       = is_global_pooling ? output_shape[idx_width] : pool_info.pool_size().width;
    const unsigned int pool_size_y       = is_global_pooling ? output_shape[idx_height] : pool_info.pool_size().height;

    std::tie(pooled_w, pooled_h) = scaled_dimensions(output_shape[idx_width],
                                                     output_shape[idx_height],
                                                     pool_size_x,
                                                     pool_size_y,
                                                     pool_info.pad_stride_info());

    output_shape.set(idx_width, pooled_w);
    output_shape.set(idx_height, pooled_h);

    return output_shape;
}

inline TensorShape compute_rnn_shape(const ITensorInfo *input, const unsigned int batch_size)
{
    TensorShape output_shape{ input->tensor_shape() };
    output_shape.set(1, batch_size);

    return output_shape;
}
inline TensorShape compute_mm_shape(const ITensorInfo &input0, const ITensorInfo &input1, bool is_interleaved_transposed, const GEMMReshapeInfo &reshape_info)
{
    ARM_COMPUTE_ERROR_ON_MSG(input0.num_dimensions() > 4, "The number of dimensions for the matrix A must be <= 4");
    ARM_COMPUTE_ERROR_ON_MSG(is_interleaved_transposed && reshape_info.reinterpret_input_as_3d(), "The first input tensor cannot be reinterpreted as 3D if is_interleaved_transposed is true");

    const bool reinterpret_input_as_3d  = reshape_info.reinterpret_input_as_3d();
    const bool reinterpret_output_as_3d = reshape_info.depth_output_gemm3d() != 1;
    const int  m                        = reshape_info.reinterpret_input_as_3d() ? input0.dimension(1) * input0.dimension(2) : input0.dimension(1);

    // If the output of GEMM has to be reinterpreted as 3D, the number of input0 rows (M) is obtained collapsing the second and third
    // dimension of the output tensor
    const int dim0 = is_interleaved_transposed ? reshape_info.n() : input1.dimension(0);
    const int dim1 = is_interleaved_transposed ? reshape_info.m() / reshape_info.depth_output_gemm3d() : m / reshape_info.depth_output_gemm3d();
    const int dim2 = reinterpret_input_as_3d ? input0.tensor_shape()[3] : input0.tensor_shape()[2];
    const int dim3 = reinterpret_input_as_3d ? 1 : input0.tensor_shape()[3];

    TensorShape output_shape{ input0.tensor_shape() };

    output_shape.set(0, dim0);
    output_shape.set(1, dim1);
    output_shape.set(2, reinterpret_output_as_3d ? reshape_info.depth_output_gemm3d() : dim2);
    output_shape.set(3, reinterpret_output_as_3d ? dim2 : dim3);
    output_shape.set(4, reinterpret_output_as_3d ? dim3 : 1);

    return output_shape;
}

template <typename T>
inline TensorShape extract_shape(T *data)
{
    return data->info()->tensor_shape();
}

inline TensorShape extract_shape(ITensorInfo *data)
{
    return data->tensor_shape();
}

inline TensorShape extract_shape(const TensorShape *data)
{
    return *data;
}

template <typename T>
inline TensorShape calculate_depth_concatenate_shape(const std::vector<T *> &inputs_vector)
{
    TensorShape out_shape = extract_shape(inputs_vector[0]);

    size_t max_x = 0;
    size_t max_y = 0;
    size_t depth = 0;

    for(const auto &tensor : inputs_vector)
    {
        ARM_COMPUTE_ERROR_ON(tensor == nullptr);
        const TensorShape shape = extract_shape(tensor);
        max_x                   = std::max(shape.x(), max_x);
        max_y                   = std::max(shape.y(), max_y);
        depth += shape.z();
    }

    out_shape.set(0, max_x);
    out_shape.set(1, max_y);
    out_shape.set(2, depth);

    return out_shape;
}

template <typename T>
inline TensorShape calculate_width_concatenate_shape(const std::vector<T *> &inputs_vector)
{
    TensorShape out_shape = extract_shape(inputs_vector[0]);

    size_t width = 0;
    for(const auto &tensor : inputs_vector)
    {
        ARM_COMPUTE_ERROR_ON(tensor == nullptr);
        const TensorShape shape = extract_shape(tensor);
        width += shape.x();
    }

    out_shape.set(0, width);

    return out_shape;
}
} // namespace shape_calculator
} // namespace misc
} // namespace arm_compute
#endif /* __ARM_COMPUTE_MISC_SHAPE_CALCULATOR_H__ */
