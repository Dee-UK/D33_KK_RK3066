static rmii_extclk_sel = 0;
static int rk30_vmac_register_set(void)
{
	int value;
	//config rk30 vmac as rmii
	writel_relaxed(0x3 << 16 | 0x2, RK30_GRF_BASE + GRF_SOC_CON1);
        value = readl(RK30_GRF_BASE + GRF_SOC_CON2);
        writel(0x1 << 6 | 0x1 << 22 | value, RK30_GRF_BASE + GRF_SOC_CON2);
	return 0;
}

static int rk30_rmii_io_init(void)
{
	int err;
	rk30_mux_api_set(GPIO1D6_CIF1DATA11_NAME, GPIO1D_GPIO1D6);

	rk30_mux_api_set(GPIO1D2_CIF1CLKIN_NAME, GPIO1D_GPIO1D2);
	rk30_mux_api_set(GPIO1D1_CIF1HREF_MIIMDCLK_NAME, GPIO1D_MII_MDCLK);
	rk30_mux_api_set(GPIO1D0_CIF1VSYNC_MIIMD_NAME, GPIO1D_MII_MD);

	rk30_mux_api_set(GPIO1C7_CIFDATA9_RMIIRXD0_NAME, GPIO1C_RMII_RXD0);
	rk30_mux_api_set(GPIO1C6_CIFDATA8_RMIIRXD1_NAME, GPIO1C_RMII_RXD1);
	rk30_mux_api_set(GPIO1C5_CIFDATA7_RMIICRSDVALID_NAME, GPIO1C_RMII_CRS_DVALID);
	rk30_mux_api_set(GPIO1C4_CIFDATA6_RMIIRXERR_NAME, GPIO1C_RMII_RX_ERR);
	rk30_mux_api_set(GPIO1C3_CIFDATA5_RMIITXD0_NAME, GPIO1C_RMII_TXD0);
	rk30_mux_api_set(GPIO1C2_CIF1DATA4_RMIITXD1_NAME, GPIO1C_RMII_TXD1);
	rk30_mux_api_set(GPIO1C1_CIFDATA3_RMIITXEN_NAME, GPIO1C_RMII_TX_EN);
	rk30_mux_api_set(GPIO1C0_CIF1DATA2_RMIICLKOUT_RMIICLKIN_NAME, GPIO1C_RMII_CLKOUT);

	//phy power gpio
	err = gpio_request(PHY_PWR_EN_GPIO, "phy_power_en");
	if (err) {
		return -1;
	}
	//phy power down
	gpio_direction_output(PHY_PWR_EN_GPIO, GPIO_LOW);
	gpio_set_value(PHY_PWR_EN_GPIO, GPIO_LOW);
	return 0;
}

static int rk30_rmii_io_deinit(void)
{
	//phy power down
	gpio_direction_output(PHY_PWR_EN_GPIO, GPIO_LOW);
	gpio_set_value(PHY_PWR_EN_GPIO, GPIO_LOW);
	//free
	gpio_free(PHY_PWR_EN_GPIO);
	return 0;
}

static int rk30_rmii_power_control(int enable)
{
    struct regulator *ldo_rmii=NULL;
    
#if defined (CONFIG_MFD_WM831X_I2C)
    if(g_pmic_type == PMIC_TYPE_WM8326)
        ldo_rmii = regulator_get(NULL, "ldo9");
#endif
#if defined (CONFIG_MFD_TPS65910)
    if(g_pmic_type == PMIC_TYPE_TPS65910)
        ldo_rmii = regulator_get(NULL, "vaux33");
#endif  
	if (ldo_rmii == NULL || IS_ERR(ldo_rmii)){
	  printk("get rmii ldo failed!\n");
	}

	if (enable) {
		//enable phy power
		printk("power on phy\n");
		rk30_mux_api_set(GPIO1D6_CIF1DATA11_NAME, GPIO1D_GPIO1D6);

		rk30_mux_api_set(GPIO1D2_CIF1CLKIN_NAME, GPIO1D_GPIO1D2);
		rk30_mux_api_set(GPIO1D1_CIF1HREF_MIIMDCLK_NAME, GPIO1D_MII_MDCLK);
		rk30_mux_api_set(GPIO1D0_CIF1VSYNC_MIIMD_NAME, GPIO1D_MII_MD);

		rk30_mux_api_set(GPIO1C7_CIFDATA9_RMIIRXD0_NAME, GPIO1C_RMII_RXD0);
		rk30_mux_api_set(GPIO1C6_CIFDATA8_RMIIRXD1_NAME, GPIO1C_RMII_RXD1);
		rk30_mux_api_set(GPIO1C5_CIFDATA7_RMIICRSDVALID_NAME, GPIO1C_RMII_CRS_DVALID);
		rk30_mux_api_set(GPIO1C4_CIFDATA6_RMIIRXERR_NAME, GPIO1C_RMII_RX_ERR);
		rk30_mux_api_set(GPIO1C3_CIFDATA5_RMIITXD0_NAME, GPIO1C_RMII_TXD0);
		rk30_mux_api_set(GPIO1C2_CIF1DATA4_RMIITXD1_NAME, GPIO1C_RMII_TXD1);
		rk30_mux_api_set(GPIO1C1_CIFDATA3_RMIITXEN_NAME, GPIO1C_RMII_TX_EN);
		rk30_mux_api_set(GPIO1C0_CIF1DATA2_RMIICLKOUT_RMIICLKIN_NAME, GPIO1C_RMII_CLKOUT);
		if(ldo_rmii && (!regulator_is_enabled(ldo_rmii))) {
		//regulator_set_voltage(ldo_rmii, 3300000, 3300000);
		  regulator_enable(ldo_rmii);
		  regulator_put(ldo_rmii);
		  mdelay(500);
		}
		gpio_direction_output(PHY_PWR_EN_GPIO, GPIO_HIGH);
		gpio_set_value(PHY_PWR_EN_GPIO, GPIO_LOW);
                mdelay(20);
		gpio_set_value(PHY_PWR_EN_GPIO, GPIO_HIGH);
	}else {
		gpio_direction_output(PHY_PWR_EN_GPIO, GPIO_LOW);
		gpio_set_value(PHY_PWR_EN_GPIO, GPIO_LOW);
		if(ldo_rmii && regulator_is_enabled(ldo_rmii)) {
		  regulator_disable(ldo_rmii);
		  regulator_put(ldo_rmii);
		  mdelay(500);
		}
	}
	return 0;
}

#define BIT_EMAC_SPEED      (1 << 1)
static int rk29_vmac_speed_switch(int speed)
{
	//printk("%s--speed=%d\n", __FUNCTION__, speed);
	if (10 == speed) {
	    writel_relaxed((readl_relaxed(RK30_GRF_BASE + GRF_SOC_CON1) | (2<<16)) & (~BIT_EMAC_SPEED), RK30_GRF_BASE + GRF_SOC_CON1);
	} else {
	    writel_relaxed(readl_relaxed(RK30_GRF_BASE + GRF_SOC_CON1) | (2<<16) | ( BIT_EMAC_SPEED), RK30_GRF_BASE + GRF_SOC_CON1);
	}
}

static int rk30_rmii_extclk_sel(void)
{
#ifdef RMII_EXT_CLK
    rmii_extclk_sel = 1; //0:select internal divider clock, 1:select external input clock
#endif 
    return rmii_extclk_sel; 
}

struct rk29_vmac_platform_data board_vmac_data = {
	.vmac_register_set = rk30_vmac_register_set,
	.rmii_io_init = rk30_rmii_io_init,
	.rmii_io_deinit = rk30_rmii_io_deinit,
	.rmii_power_control = rk30_rmii_power_control,
	.rmii_speed_switch = rk29_vmac_speed_switch,
   .rmii_extclk_sel = rk30_rmii_extclk_sel,
};
