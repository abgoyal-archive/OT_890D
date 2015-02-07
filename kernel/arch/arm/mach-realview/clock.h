
struct module;
struct icst307_params;

struct clk {
	unsigned long		rate;
	const struct icst307_params *params;
	void			*data;
	void			(*setvco)(struct clk *, struct icst307_vco vco);
};
