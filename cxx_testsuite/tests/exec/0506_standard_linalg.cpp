// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <complex>
#include <execution>
#include <linalg>
#include <mdspan>
#include <type_traits>
#include <version>

#if __cpp_lib_linalg != 202311L
#error "unexpected __cpp_lib_linalg value"
#endif

int main() {
  int x_data[3] = {1, 2, 3};
  int y_data[3] = {4, 5, 6};
  int z_data[3] = {};
  std::mdspan<int, std::extents<std::size_t, 3>> x(x_data);
  std::mdspan<int, std::extents<std::size_t, 3>> y(y_data);
  std::mdspan<int, std::extents<std::size_t, 3>> z(z_data);

  if (std::linalg::dot(x, y) != 32 ||
      std::linalg::vector_abs_sum(x) != 6 ||
      std::linalg::vector_idx_abs_max(x) != 2) {
    return 1;
  }
#if STOP == 1
  return 0;
#endif
  std::linalg::add(x, y, z);
  std::linalg::scale(std::execution::seq, 2, z);
  if (z[0] != 10 || z[1] != 14 || z[2] != 18) {
    return 2;
  }
#if STOP == 2
  return 0;
#endif
  std::linalg::swap_elements(x, y);
  if (x[0] != 4 || y[2] != 3) {
    return 3;
  }
#if STOP == 3
  return 0;
#endif

  int a_data[6] = {1, 2, 3, 4, 5, 6};
  int v_data[3] = {1, 2, 1};
  int out_data[2] = {};
  std::mdspan<int, std::extents<std::size_t, 2, 3>> a(a_data);
  std::mdspan<int, std::extents<std::size_t, 3>> v(v_data);
  std::mdspan<int, std::extents<std::size_t, 2>> out(out_data);
  std::linalg::matrix_vector_product(a, v, out);
  if (out[0] != 8 || out[1] != 20) {
    return 4;
  }
#if STOP == 4
  return 0;
#endif

  auto at = std::linalg::transposed(a);
  using at_layout = decltype(at)::layout_type;
  static_assert(std::is_same<at_layout, std::layout_left>::value);
  if (at.extent(0) != 3 || at.extent(1) != 2 ||
      at[2, 1] != 6) {
    return 5;
  }
  auto att = std::linalg::transposed(at);
  using att_layout = decltype(att)::layout_type;
  static_assert(std::is_same<att_layout, std::layout_right>::value);
  if (att[1, 2] != 6) {
    return 18;
  }
#if STOP == 45
  return 0;
#endif
  auto twice_at = std::linalg::scaled(2, at);
  if (twice_at[1, 0] != 4) {
    return 5;
  }
#if STOP == 5
  return 0;
#endif

  int b_data[6] = {1, 2, 3, 4, 5, 6};
  int c_data[4] = {};
  std::mdspan<int, std::extents<std::size_t, 3, 2>> b(b_data);
  std::mdspan<int, std::extents<std::size_t, 2, 2>> c(c_data);
  std::linalg::matrix_product(a, b, c);
  if (c[0, 0] != 22 || c[0, 1] != 28 ||
      c[1, 0] != 49 || c[1, 1] != 64) {
    return 6;
  }
#if STOP == 6
  return 0;
#endif

  using packed_extents = std::extents<std::size_t, 3, 3>;
  using packed_layout = std::linalg::layout_blas_packed<
      std::linalg::lower_triangle_t, std::linalg::row_major_t>;
  packed_extents packed_shape;
  packed_layout::mapping<packed_extents> packed_mapping(packed_shape);
  if (packed_mapping.required_span_size() != 6 ||
      packed_mapping(0, 0) != 0 || packed_mapping(1, 0) != 1 ||
      packed_mapping(1, 1) != 2 || packed_mapping(2, 0) != 3 ||
      packed_mapping(0, 2) != packed_mapping(2, 0)) {
    return 9;
  }
  double packed_data[6] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  std::mdspan<double, packed_extents, packed_layout> packed(
      packed_data, packed_mapping);
  auto packed_transpose = std::linalg::transposed(packed);
  if (packed_transpose[0, 2] != packed[2, 0]) {
    return 19;
  }

  double gx_data[1] = {3.0};
  double gy_data[1] = {4.0};
  std::mdspan<double, std::extents<std::size_t, 1>> gx(gx_data);
  std::mdspan<double, std::extents<std::size_t, 1>> gy(gy_data);
  auto rotation = std::linalg::setup_givens_rotation(3.0, 4.0);
  std::linalg::apply_givens_rotation(gx, gy, rotation.c, rotation.s);
  if (rotation.r != 5.0 || gx[0] < 4.999999999999 ||
      gx[0] > 5.000000000001 || gy[0] < -0.000000000001 ||
      gy[0] > 0.000000000001) {
    return 10;
  }

  double sa_data[4] = {1.0, 2.0, 0.0, 3.0};
  double sx_data[2] = {1.0, 2.0};
  double sy_data[2] = {};
  std::mdspan<double, std::extents<std::size_t, 2, 2>> sa(sa_data);
  std::mdspan<double, std::extents<std::size_t, 2>> sx(sx_data);
  std::mdspan<double, std::extents<std::size_t, 2>> sy(sy_data);
  std::linalg::symmetric_matrix_vector_product(
      sa, std::linalg::upper_triangle, sx, sy);
  if (sy[0] != 5.0 || sy[1] != 8.0) {
    return 11;
  }

  double ta_data[4] = {2.0, 0.0, 3.0, 4.0};
  double tx_data[2] = {1.0, 2.0};
  double ty_data[2] = {};
  std::mdspan<double, std::extents<std::size_t, 2, 2>> ta(ta_data);
  std::mdspan<double, std::extents<std::size_t, 2>> tx(tx_data);
  std::mdspan<double, std::extents<std::size_t, 2>> ty(ty_data);
  std::linalg::triangular_matrix_vector_product(
      ta, std::linalg::lower_triangle, std::linalg::explicit_diagonal,
      tx, ty);
  if (ty[0] != 2.0 || ty[1] != 11.0) {
    return 12;
  }
  std::linalg::triangular_matrix_vector_solve(
      ta, std::linalg::lower_triangle, std::linalg::explicit_diagonal, ty);
  if (ty[0] != 1.0 || ty[1] != 2.0) {
    return 13;
  }

  double identity_data[4] = {1.0, 0.0, 0.0, 1.0};
  double product_data[4] = {};
  std::mdspan<double, std::extents<std::size_t, 2, 2>> identity(
      identity_data);
  std::mdspan<double, std::extents<std::size_t, 2, 2>> structured_product(
      product_data);
  std::linalg::symmetric_matrix_product(
      sa, std::linalg::upper_triangle, identity, structured_product);
  if (structured_product[0, 0] != 1.0 ||
      structured_product[0, 1] != 2.0 ||
      structured_product[1, 0] != 2.0 ||
      structured_product[1, 1] != 3.0) {
    return 14;
  }

  double solve_data[4] = {2.0, 0.0, 3.0, 4.0};
  std::mdspan<double, std::extents<std::size_t, 2, 2>> solve_matrix(
      solve_data);
  std::linalg::triangular_matrix_matrix_left_solve(
      ta, std::linalg::lower_triangle, std::linalg::explicit_diagonal,
      solve_matrix);
  if (solve_matrix[0, 0] != 1.0 || solve_matrix[0, 1] != 0.0 ||
      solve_matrix[1, 0] != 0.0 || solve_matrix[1, 1] != 1.0) {
    return 15;
  }

  double rank_vector_data[2] = {1.0, 2.0};
  double rank_matrix_data[4] = {9.0, 9.0, 9.0, 9.0};
  std::mdspan<double, std::extents<std::size_t, 2>> rank_vector(
      rank_vector_data);
  std::mdspan<double, std::extents<std::size_t, 2, 2>> rank_matrix(
      rank_matrix_data);
  std::linalg::symmetric_matrix_rank_1_update(
      1.0, rank_vector, rank_matrix, std::linalg::upper_triangle);
  if (rank_matrix[0, 0] != 1.0 || rank_matrix[0, 1] != 2.0 ||
      rank_matrix[1, 1] != 4.0) {
    return 16;
  }
  std::linalg::symmetric_matrix_rank_1_update(
      1.0, rank_vector, rank_matrix, rank_matrix,
      std::linalg::upper_triangle);
  if (rank_matrix[0, 0] != 2.0 || rank_matrix[0, 1] != 4.0 ||
      rank_matrix[1, 1] != 8.0) {
    return 22;
  }
  double rank_k_input_data[2] = {1.0, 2.0};
  std::mdspan<double, std::extents<std::size_t, 2, 1>> rank_k_input(
      rank_k_input_data);
  std::linalg::symmetric_matrix_rank_k_update(
      1.0, rank_k_input, rank_matrix, std::linalg::upper_triangle);
  if (rank_matrix[0, 0] != 1.0 || rank_matrix[0, 1] != 2.0 ||
      rank_matrix[1, 1] != 4.0) {
    return 23;
  }
  if (std::linalg::vector_two_norm(std::execution::seq, rank_vector) !=
      std::sqrt(5.0)) {
    return 17;
  }
  if (std::linalg::vector_two_norm(rank_vector, 2.0) != 3.0 ||
      std::linalg::matrix_frob_norm(identity, 2.0) != std::sqrt(6.0)) {
    return 20;
  }

  std::complex<double> p_data[2] = {{1.0, 1.0}, {2.0, -1.0}};
  std::complex<double> q_data[2] = {{2.0, 0.0}, {1.0, 1.0}};
  std::mdspan<std::complex<double>, std::extents<std::size_t, 2>> p(p_data);
  std::mdspan<std::complex<double>, std::extents<std::size_t, 2>> q(q_data);
  auto d = std::linalg::dotc(p, q);
  if (d != std::complex<double>(3.0, 1.0)) {
    return 7;
  }
  auto pc = std::linalg::conjugated(p);
  if (pc[0] != std::complex<double>(1.0, -1.0)) {
    return 8;
  }
  if (std::linalg::vector_abs_sum(p) != std::complex<double>(5.0, 0.0) ||
      std::linalg::vector_idx_abs_max(p) != 1) {
    return 21;
  }
  return 0;
}
