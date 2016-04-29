#include "p2p_parameter.h"

P2pParameter g_p2p_param;

SHCDNPtType  g_pt_map[PLATFORM_ANDROID_TV];

P2pParameter::P2pParameter()
{
    g_pt_map[PLATFORM_PC] = kSHPlatform_PC;
    g_pt_map[PLATFORM_MAC] = kSHPlatform_MAC;
    g_pt_map[PLATFORM_IPHONE] = kSHPlatform_IPHONE;
    g_pt_map[PLATFORM_IPAD] = kSHPlatform_IPAD;
    g_pt_map[PLATFORM_ANDROID_PHONE] = kSHPlatform_AndroidPhone;
    g_pt_map[PLATFORM_ANDROID_PAD] = kSHPlatform_AndroidPad;
    g_pt_map[PLATFORM_ANDROID_TV] = kSHPlatform_AndroidTV;

    send_time_out_	   = 6;
    start_section_cdn_num_ = 3;
    peer_speed_less_use_cdn_=40;
    use_peer_cdn_speed_less_ = 30;

    urgent_to_p2p_time_=21;
    urgent_top_p2p_num_=5;
    urgent_to_state_time_= 4;
    urgent_p2p_to_http_time_=3;
    urgent_p2p_to_http_time_delay_=8;
    urgent_p2p_to_http_speed_=10 * 1024;
    urgent_mix_to_p2p_speed_= 10 * 1024;
    play_to_state_time_=2;
    play_to_http_speed_=10*1024;
    play_http_to_p2p_time_=5;
    play_p2p_to_http_speed_=1024;
    play_mix_to_p2p_speed_=20*1024;
    speed_limit_for_working_=5;
    speed_limit_for_stay_=2;
    speed_limit_for_full_screen_=2;
    notify_buffer_time_=50;
    check_connect_status_interval=1;
    urgent_coefficient_=4;
    force_to_http_time=15;
    p2p_to_http_peer_num_=10;
    http_speed_coefficient_=3;
    refetch_peer_=1;

    gaps_seconds_ = 14;
    supply_peers_num_ = 3;
    max_peer_conn_ = 18;
    prior_value_factor_ = 30;            // 1~100
    
    http_startup_speed_factor_ = 80;     //大于30的话，要调整timeout判断条件
    p2p_speed_limit_factor_ = 6;

    share_file_ = 0;
    max_upload_speed_ = 320;
    max_up_connect_ = 40;

	download_p2p_to_http_min_speed_ = 80;
	download_p2p_to_http_speed_when_few_peer_ = 120;
    download_http_to_p2p_when_http_speed_less_ = 50;

	p2p_speed_leave_only_p2p_in_comb_status_ = 100;
	p2p_speed_leave_only_http_in_comb_status_ = 10;

	p2p_feed_hunger_stategy_ = 1;
	p2p_only_feed_download_hunger_ = 1;

    unblock_punch_ = 1;

#ifdef __IOS__
	urgent_concurrent_download_mode_ = 1;
	urgent_preload_time_ = 22;
#else
	urgent_concurrent_download_mode_ = 0;
	urgent_preload_time_ = 0;
#endif

#ifdef ENABLE_FLASH_P2P
	p2p_download_peer_count_ = 30;
	max_flash_peer_connection_ = 30;
	enable_flash_p2p_ = 1;
	flash_target_speed = 1024 * 1024;
    max_piece_per_second_ = 4;
#endif // #ifdef ENABLE_FLASH_P2P

#ifdef ENABLE_LUA_STATE_MACHINE
	enable_lua_state_machine_ = 1;
	lua_state_machine_count_ = 3;
	lua_state_machine_version_ = 2;
	force_overwrite_lua_state_machine_ = 1;
	lua_state_machine_url_ = "http://10.16.14.183:8088/StateMachine.lua";
#endif // #ifdef ENABLE_LUA_STATE_MACHINE

	default_sp_code_ = 2;
	default_city_code_ = 40;

    if_vrs_iplocate_ = true;

#ifdef ENABLE_GATEWAY
    gateway_reconnect_interval_ = 30;
    gateway_update_interval_ = 30 * 60;
    gateway_disconnect_timeout_ = 6;
#endif // #ifdef ENABLE_GATEWAY

#ifdef ENABLE_CACHE_PEER
    enable_cache_ifox_peer_ = 0;
    enable_cache_flash_peer_ = 1;
    cached_peer_life_time_ = 10;
#endif // #ifdef ENABLE_CACHE_PEER

    p2p_sync_non_block_read_count_ = 10;
}
