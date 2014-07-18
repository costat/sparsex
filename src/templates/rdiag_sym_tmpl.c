static inline spx_value_t rdiag${delta}_case(
    uint8_t **ctl, uint8_t size, spx_value_t **values,
    spx_value_t *x, spx_value_t *y, spx_value_t *cur,
    spx_index_t *x_indx, spx_index_t *y_indx, spx_value_t scale_f)
{
	register spx_value_t values_ = **values;
	register spx_index_t i_end = ${delta} * size;
	register spx_index_t x_indx_ = *x_indx;
	register spx_value_t *x_ = x + x_indx_ - i_end;
	register spx_value_t *ry_ = cur + x_indx_ - i_end;
	register spx_index_t y_indx_ = *y_indx;
	register spx_value_t *y_ = y + y_indx_;
	register spx_value_t *rx_ = x + y_indx_;
	spx_index_t i, j;

	for (i = 0, j = i_end; i < i_end; i += ${delta}, j -= ${delta}) {
		y_[i] += x_[j] * values_ * scale_f;
		ry_[j] += rx_[i] * values_ * scale_f;
		(*values)++;
		values_ = **values;
	}

	return 0;
}