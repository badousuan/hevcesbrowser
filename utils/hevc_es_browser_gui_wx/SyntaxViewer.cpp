#include "SyntaxViewer.h"
#include <wx/treectrl.h>
#include <wx/clipbrd.h>
#include <wx/menu.h>
#include <HevcUtils.h>
#include <ConvToString.h>

#define SLICE_B 0
#define SLICE_P 1
#define SLICE_I 2

SyntaxViewer::SyntaxViewer(wxWindow* parent)
    : wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT)
{
    Bind(wxEVT_TREE_ITEM_COLLAPSED, [this](wxTreeEvent& event) {
        m_state.setActive(GetItemText(event.GetItem()), false);
    });
    Bind(wxEVT_TREE_ITEM_EXPANDED, [this](wxTreeEvent& event) {
        m_state.setActive(GetItemText(event.GetItem()), true);
    });

    Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &SyntaxViewer::onRightClick, this);
    Bind(wxEVT_KEY_DOWN, &SyntaxViewer::onKeyDown, this);
}

void SyntaxViewer::setParameretsSets(const VPSMap &vpsMap, const SPSMap &spsMap, const PPSMap &ppsMap)
{
    m_vpsMap = vpsMap;
    m_spsMap = spsMap;
    m_ppsMap = ppsMap;
}

void SyntaxViewer::onNalUChanged(std::shared_ptr<HEVC::NALUnit> pNalU, ParserInfo info)
{
    DeleteAllItems();
    AddRoot("NALU");
    
    using namespace HEVC;
    switch(pNalU -> m_nalHeader.type)
    {
        case NAL_VPS:
        {
            std::shared_ptr<HEVC::VPS> pVPS = std::dynamic_pointer_cast<HEVC::VPS>(pNalU);
            createVPS(pVPS);
            break;
        }

        case NAL_SPS:
        {
            std::shared_ptr<HEVC::SPS> pSPS = std::dynamic_pointer_cast<HEVC::SPS>(pNalU);
            createSPS(pSPS);
            break;
        }

        case NAL_PPS:
        {
            std::shared_ptr<HEVC::PPS> pPPS = std::dynamic_pointer_cast<HEVC::PPS>(pNalU);
            createPPS(pPPS);
            break;
        }

        case NAL_TRAIL_R:
        case NAL_TRAIL_N:
        case NAL_TSA_N:
        case NAL_TSA_R:
        case NAL_STSA_N:
        case NAL_STSA_R:
        case NAL_BLA_W_LP:
        case NAL_BLA_W_RADL:
        case NAL_BLA_N_LP:
        case NAL_IDR_W_RADL:
        case NAL_IDR_N_LP:
        case NAL_CRA_NUT:
        case NAL_RADL_N:
        case NAL_RADL_R:
        case NAL_RASL_N:
        case NAL_RASL_R:
        {
            createSlice(std::dynamic_pointer_cast<HEVC::Slice>(pNalU));
            break;
        }

        case NAL_AUD:
        {
            createAUD(std::dynamic_pointer_cast<HEVC::AUD>(pNalU));
            break;
        }

        case NAL_SEI_PREFIX:
        case NAL_SEI_SUFFIX:
        {
            createSEI(std::dynamic_pointer_cast<HEVC::SEI>(pNalU));
            break;
        }

        default:
            break;
    };

    updateItemsState();
}

void SyntaxViewer::createVPS(std::shared_ptr<HEVC::VPS> pVPS)
{
    wxTreeItemId root = GetRootItem();
    wxTreeItemId pvpsItem;
    wxTreeItemId pitem;
    wxTreeItemId ploop;
    wxTreeItemId pitemSecond;
    wxTreeItemId pitemThird;

    pvpsItem = AppendItem(root, "VPS");

    AppendItem(pvpsItem, wxString::Format("vps_video_parameter_set_id = %lld", (long long)pVPS -> vps_video_parameter_set_id));
    AppendItem(pvpsItem, wxString::Format("vps_max_layers_minus1 = %lld", (long long)pVPS -> vps_max_layers_minus1));
    AppendItem(pvpsItem, wxString::Format("vps_max_sub_layers_minus1 = %lld", (long long)pVPS -> vps_max_sub_layers_minus1));
    AppendItem(pvpsItem, wxString::Format("vps_temporal_id_nesting_flag = %lld", (long long)pVPS -> vps_temporal_id_nesting_flag));

    pitem = AppendItem(pvpsItem, "profile_tier_level");
    createProfileTierLevel(pVPS -> profile_tier_level, pitem);

    AppendItem(pvpsItem, wxString::Format("vps_sub_layer_ordering_info_present_flag = %lld", (long long)pVPS -> vps_sub_layer_ordering_info_present_flag));

    ploop = AppendItem(pvpsItem, "for( i = ( vps_sub_layer_ordering_info_present_flag ? 0 : vps_max_sub_layers_minus1 ); i <= vps_max_sub_layers_minus1; i++ )");

    for(std::size_t i = (pVPS -> vps_sub_layer_ordering_info_present_flag ? 0 : pVPS -> vps_max_sub_layers_minus1); i <= pVPS -> vps_max_sub_layers_minus1; i++)
    {
        AppendItem(ploop, wxString::Format("vps_max_dec_pic_buffering_minus1[%lld] = %lld", (long long)i, (long long)pVPS -> vps_max_dec_pic_buffering_minus1[i]));
        AppendItem(ploop, wxString::Format("vps_max_num_reorder_pics[%lld] = %lld", (long long)i, (long long)pVPS -> vps_max_num_reorder_pics[i]));
        AppendItem(ploop, wxString::Format("vps_max_latency_increase_plus1[%lld] = %lld", (long long)i, (long long)pVPS -> vps_max_latency_increase_plus1[i]));
    }

    AppendItem(pvpsItem, wxString::Format("vps_max_layer_id = %lld", (long long)pVPS -> vps_max_layer_id));
    AppendItem(pvpsItem, wxString::Format("vps_num_layer_sets_minus1 = %lld", (long long)pVPS -> vps_num_layer_sets_minus1));


    if(pVPS -> vps_num_layer_sets_minus1 == 0)
    {
        AppendItem(pvpsItem, "layer_id_included_flag = { }");
    }
    else
    {
        ploop = AppendItem(pvpsItem, "for(std::size_t i=0; i<vps_num_layer_sets_minus1; i++)");

        for(std::size_t i=0; i<pVPS -> vps_num_layer_sets_minus1; i++)
        {
            wxString str;

            if(pVPS -> vps_max_layer_id == 0)
                str = wxString::Format("layer_id_included_flag[%lld] = { } ", (long long)i);
            else
            {
                str = wxString::Format("layer_id_included_flag[%lld] = { ", (long long)i);
                for(int j=0; j<pVPS -> vps_max_layer_id - 1; j++)
                {
                    str += wxString::Format("%lld,  ", (long long)pVPS -> layer_id_included_flag[i][j]);
                    if((j + 1) % 8 == 0)
                        str += "\n";
                }
                str += wxString::Format("%lld } ", (long long)pVPS -> layer_id_included_flag[i][pVPS -> vps_max_layer_id - 1]);
            }
            AppendItem(ploop, str);
        }
    }

    AppendItem(pvpsItem, wxString::Format("vps_timing_info_present_flag = %lld", (long long)pVPS -> vps_timing_info_present_flag));

    if(pVPS -> vps_timing_info_present_flag)
    {
        pitem = AppendItem(pvpsItem, "if( vps_timing_info_present_flag )");

        AppendItem(pitem, wxString::Format("vps_num_units_in_tick = %lld", (long long)pVPS -> vps_num_units_in_tick));
        AppendItem(pitem, wxString::Format("vps_time_scale = %lld", (long long)pVPS -> vps_time_scale));
        AppendItem(pitem, wxString::Format("vps_poc_proportional_to_timing_flag = %lld", (long long)pVPS -> vps_poc_proportional_to_timing_flag));

        if(pVPS -> vps_poc_proportional_to_timing_flag)
        {
            pitemSecond = AppendItem(pitem, "if( vps_poc_proportional_to_timing_flag )");
            AppendItem(pitemSecond, wxString::Format("vps_num_ticks_poc_diff_one_minus1 = %lld", (long long)pVPS -> vps_num_ticks_poc_diff_one_minus1));
        }

        AppendItem(pitem, wxString::Format("vps_num_hrd_parameters = %lld", (long long)pVPS -> vps_num_hrd_parameters));

        if(pVPS -> vps_num_hrd_parameters)
        {
            ploop = AppendItem(pitem, "for( i = 0; i < vps_num_hrd_parameters; i++ )");

            for(std::size_t i=0; i<pVPS -> vps_num_hrd_parameters; i++)
            {
                AppendItem(ploop, wxString::Format("hrd_layer_set_idx[ %lld] = %lld", (long long)i, (long long)pVPS -> hrd_layer_set_idx[i]));
                if(i > 0)
                {
                    pitemSecond = AppendItem(ploop, "if( i > 0 )");
                    AppendItem(pitemSecond, wxString::Format("cprms_present_flag[ %lld] = %lld", (long long)i, (long long)pVPS -> cprms_present_flag[i]));
                }

                pitemThird = AppendItem(ploop, wxString::Format("hrd_parameters(%lld, %lld)", (long long)i, (long long)pVPS -> vps_max_sub_layers_minus1));
                createHrdParameters(pVPS -> hrd_parameters[i], pVPS -> cprms_present_flag[i], pitemThird);
            }
        }
    }

    AppendItem(pvpsItem, wxString::Format("vps_extension_flag = %lld", (long long)pVPS -> vps_extension_flag));
}

void SyntaxViewer::createSPS(std::shared_ptr<HEVC::SPS> pSPS)
{
    wxTreeItemId root = GetRootItem();
    wxTreeItemId pspsItem;
    wxTreeItemId pitem;
    wxTreeItemId pitemSecond;
    wxTreeItemId pitemLoop;
    wxTreeItemId pVuiItem;
    wxTreeItemId pStrpc;

    pspsItem = AppendItem(root, "SPS");

    AppendItem(pspsItem, wxString::Format("sps_video_parameter_set_id = %lld", (long long)pSPS -> sps_video_parameter_set_id));
    AppendItem(pspsItem, wxString::Format("sps_max_sub_layers_minus1 = %lld", (long long)pSPS -> sps_max_sub_layers_minus1));
    AppendItem(pspsItem, wxString::Format("sps_temporal_id_nesting_flag = %lld", (long long)pSPS -> sps_temporal_id_nesting_flag));

    pitem = AppendItem(pspsItem, "profile_tier_level");
    createProfileTierLevel(pSPS -> profile_tier_level, pitem);

    AppendItem(pspsItem, wxString::Format("sps_seq_parameter_set_id = %lld", (long long)pSPS -> sps_seq_parameter_set_id));
    AppendItem(pspsItem, wxString::Format("chroma_format_idc = %lld", (long long)pSPS -> chroma_format_idc));

    if(pSPS -> chroma_format_idc == 3)
    {
        pitem = AppendItem(pspsItem, "if( chroma_format_idc == 3 )");
        AppendItem(pitem, wxString::Format("separate_colour_plane_flag = %lld", (long long)pSPS -> separate_colour_plane_flag));
    }

    AppendItem(pspsItem, wxString::Format("pic_width_in_luma_samples = %lld", (long long)pSPS -> pic_width_in_luma_samples));
    AppendItem(pspsItem, wxString::Format("pic_height_in_luma_samples = %lld", (long long)pSPS -> pic_height_in_luma_samples));
    AppendItem(pspsItem, wxString::Format("conformance_window_flag = %lld", (long long)pSPS -> conformance_window_flag));

    if(pSPS -> conformance_window_flag)
    {
        pitem = AppendItem(pspsItem, "if( conformance_window_flag )");
        AppendItem(pitem, wxString::Format("conf_win_left_offset = %lld", (long long)pSPS -> conf_win_left_offset));
        AppendItem(pitem, wxString::Format("conf_win_right_offset = %lld", (long long)pSPS -> conf_win_right_offset));
        AppendItem(pitem, wxString::Format("conf_win_top_offset = %lld", (long long)pSPS -> conf_win_top_offset));
        AppendItem(pitem, wxString::Format("conf_win_bottom_offset = %lld", (long long)pSPS -> conf_win_bottom_offset));
    }

    AppendItem(pspsItem, wxString::Format("bit_depth_luma_minus8 = %lld", (long long)pSPS -> bit_depth_luma_minus8));
    AppendItem(pspsItem, wxString::Format("bit_depth_chroma_minus8 = %lld", (long long)pSPS -> bit_depth_chroma_minus8));
    AppendItem(pspsItem, wxString::Format("log2_max_pic_order_cnt_lsb_minus4 = %lld", (long long)pSPS -> log2_max_pic_order_cnt_lsb_minus4));
    AppendItem(pspsItem, wxString::Format("sps_sub_layer_ordering_info_present_flag = %lld", (long long)pSPS -> sps_sub_layer_ordering_info_present_flag));

    pitem = AppendItem(pspsItem, "for( i = ( sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1 );i <= sps_max_sub_layers_minus1; i++ )");

    for(std::size_t i = (pSPS -> sps_sub_layer_ordering_info_present_flag ? 0 : pSPS -> sps_max_sub_layers_minus1); i <= pSPS -> sps_max_sub_layers_minus1; i++ )
    {
        AppendItem(pitem, wxString::Format("sps_max_dec_pic_buffering_minus1[%lld] = %lld", (long long)i, (long long)pSPS -> sps_max_dec_pic_buffering_minus1[i]));
        AppendItem(pitem, wxString::Format("sps_max_num_reorder_pics[%lld] = %lld", (long long)i, (long long)pSPS -> sps_max_num_reorder_pics[i]));
        AppendItem(pitem, wxString::Format("sps_max_latency_increase_plus1[%lld] = %lld", (long long)i, (long long)pSPS -> sps_max_latency_increase_plus1[i]));
    }

    AppendItem(pspsItem, wxString::Format("log2_min_luma_coding_block_size_minus3 = %lld", (long long)pSPS -> log2_min_luma_coding_block_size_minus3));
    AppendItem(pspsItem, wxString::Format("log2_diff_max_min_luma_coding_block_size = %lld", (long long)pSPS -> log2_diff_max_min_luma_coding_block_size));
    AppendItem(pspsItem, wxString::Format("log2_min_transform_block_size_minus2 = %lld", (long long)pSPS -> log2_min_transform_block_size_minus2));
    AppendItem(pspsItem, wxString::Format("log2_diff_max_min_transform_block_size = %lld", (long long)pSPS -> log2_diff_max_min_transform_block_size));
    AppendItem(pspsItem, wxString::Format("max_transform_hierarchy_depth_inter = %lld", (long long)pSPS -> max_transform_hierarchy_depth_inter));
    AppendItem(pspsItem, wxString::Format("max_transform_hierarchy_depth_intra = %lld", (long long)pSPS -> max_transform_hierarchy_depth_intra));
    AppendItem(pspsItem, wxString::Format("scaling_list_enabled_flag = %lld", (long long)pSPS -> scaling_list_enabled_flag));

    if(pSPS -> scaling_list_enabled_flag)
    {
        pitem = AppendItem(pspsItem, "if( scaling_list_enabled_flag )");
        AppendItem(pitem, wxString::Format("sps_scaling_list_data_present_flag = %lld", (long long)pSPS -> sps_scaling_list_data_present_flag));

        if(pSPS -> sps_scaling_list_data_present_flag)
        {
            pitemSecond = AppendItem(pitem, "scaling_list_data( )");
            createScalingListData(pSPS -> scaling_list_data, pitemSecond);
        }
    }

    AppendItem(pspsItem, wxString::Format("amp_enabled_flag = %lld", (long long)pSPS -> amp_enabled_flag));
    AppendItem(pspsItem, wxString::Format("sample_adaptive_offset_enabled_flag = %lld", (long long)pSPS -> sample_adaptive_offset_enabled_flag));
    AppendItem(pspsItem, wxString::Format("pcm_enabled_flag = %lld", (long long)pSPS -> pcm_enabled_flag));
    if(pSPS -> pcm_enabled_flag)
    {
        pitem = AppendItem(pspsItem, "if( pcm_enabled_flag )");

        AppendItem(pitem, wxString::Format("pcm_sample_bit_depth_luma_minus1 = %lld", (long long)pSPS -> pcm_sample_bit_depth_luma_minus1));
        AppendItem(pitem, wxString::Format("pcm_sample_bit_depth_chroma_minus1 = %lld", (long long)pSPS -> pcm_sample_bit_depth_chroma_minus1));
        AppendItem(pitem, wxString::Format("log2_min_pcm_luma_coding_block_size_minus3 = %lld", (long long)pSPS -> log2_min_pcm_luma_coding_block_size_minus3));
        AppendItem(pitem, wxString::Format("log2_diff_max_min_pcm_luma_coding_block_size = %lld", (long long)pSPS -> log2_diff_max_min_pcm_luma_coding_block_size));
        AppendItem(pitem, wxString::Format("pcm_loop_filter_disabled_flag = %lld", (long long)pSPS -> pcm_loop_filter_disabled_flag));
    }

    AppendItem(pspsItem, wxString::Format("num_short_term_ref_pic_sets = %lld", (long long)pSPS -> num_short_term_ref_pic_sets));

    if(pSPS -> num_short_term_ref_pic_sets)
    {
        pitem = AppendItem(pspsItem, "for( i = 0; i < num_short_term_ref_pic_sets; i++)");

        for(std::size_t i=0; i<pSPS -> num_short_term_ref_pic_sets; i++)
        {
            pStrpc = AppendItem(pitem, wxString::Format("short_term_ref_pic_set(%lld)", (long long)i));

            HEVC::ShortTermRefPicSet rpset = pSPS -> short_term_ref_pic_set[i];
            createShortTermRefPicSet(i, rpset, pSPS -> num_short_term_ref_pic_sets, pSPS -> short_term_ref_pic_set, pStrpc);
        }
    }


    AppendItem(pspsItem, wxString::Format("long_term_ref_pics_present_flag = %lld", (long long)pSPS -> long_term_ref_pics_present_flag));
    if(pSPS -> long_term_ref_pics_present_flag)
    {
        pitem = AppendItem(pspsItem, "if( long_term_ref_pics_present_flag )");

        AppendItem(pitem, wxString::Format("num_long_term_ref_pics_sps = %lld", (long long)pSPS -> num_long_term_ref_pics_sps));


        if(pSPS -> num_long_term_ref_pics_sps > 0)
        {
            pitemLoop = AppendItem(pitem, "for( i = 0; i < num_long_term_ref_pics_sps; i++ )");

            for(std::size_t i=0; i<pSPS -> num_long_term_ref_pics_sps; i++)
            {
                AppendItem(pspsItem, wxString::Format("lt_ref_pic_poc_lsb_sps[ %lld] = %lld", (long long)i, (long long)pSPS -> lt_ref_pic_poc_lsb_sps[i]));
                AppendItem(pspsItem, wxString::Format("used_by_curr_pic_lt_sps_flag[ %lld] = %lld", (long long)i, (long long)pSPS -> used_by_curr_pic_lt_sps_flag[i]));
            }
        }
    }

    AppendItem(pspsItem, wxString::Format("sps_temporal_mvp_enabled_flag = %lld", (long long)pSPS -> sps_temporal_mvp_enabled_flag));
    AppendItem(pspsItem, wxString::Format("strong_intra_smoothing_enabled_flag = %lld", (long long)pSPS -> strong_intra_smoothing_enabled_flag));
    AppendItem(pspsItem, wxString::Format("vui_parameters_present_flag = %lld", (long long)pSPS -> vui_parameters_present_flag));

    if(pSPS -> vui_parameters_present_flag)
    {
        pitem = AppendItem(pspsItem, "if( vui_parameters_present_flag )");
        pVuiItem = AppendItem(pitem, "vui_parameters");

        createVuiParameters(pSPS -> vui_parameters, pSPS -> sps_max_sub_layers_minus1, pVuiItem);
    }

    AppendItem(pspsItem, wxString::Format("sps_extension_flag = %lld", (long long)pSPS -> sps_extension_flag));
}

void SyntaxViewer::createPPS(std::shared_ptr<HEVC::PPS> pPPS)
{
    wxTreeItemId root = GetRootItem();
    wxTreeItemId pppsItem;
    wxTreeItemId pitem;
    wxTreeItemId pitemSecond;

    pppsItem = AppendItem(root, "PPS");

    AppendItem(pppsItem, wxString::Format("pps_pic_parameter_set_id = %lld", (long long)pPPS -> pps_pic_parameter_set_id));
    AppendItem(pppsItem, wxString::Format("pps_seq_parameter_set_id = %lld", (long long)pPPS -> pps_seq_parameter_set_id));
    AppendItem(pppsItem, wxString::Format("dependent_slice_segments_enabled_flag = %lld", (long long)pPPS -> dependent_slice_segments_enabled_flag));
    AppendItem(pppsItem, wxString::Format("output_flag_present_flag = %lld", (long long)pPPS -> output_flag_present_flag));
    AppendItem(pppsItem, wxString::Format("num_extra_slice_header_bits = %lld", (long long)pPPS -> num_extra_slice_header_bits));
    AppendItem(pppsItem, wxString::Format("sign_data_hiding_flag = %lld", (long long)pPPS -> sign_data_hiding_flag));
    AppendItem(pppsItem, wxString::Format("cabac_init_present_flag = %lld", (long long)pPPS -> cabac_init_present_flag));
    AppendItem(pppsItem, wxString::Format("num_ref_idx_l0_default_active_minus1 = %lld", (long long)pPPS -> num_ref_idx_l0_default_active_minus1));
    AppendItem(pppsItem, wxString::Format("num_ref_idx_l1_default_active_minus1 = %lld", (long long)pPPS -> num_ref_idx_l1_default_active_minus1));
    AppendItem(pppsItem, wxString::Format("init_qp_minus26 = %lld", (long long)pPPS -> init_qp_minus26));
    AppendItem(pppsItem, wxString::Format("constrained_intra_pred_flag = %lld", (long long)pPPS -> constrained_intra_pred_flag));
    AppendItem(pppsItem, wxString::Format("transform_skip_enabled_flag = %lld", (long long)pPPS -> transform_skip_enabled_flag));

    AppendItem(pppsItem, wxString::Format("cu_qp_delta_enabled_flag = %lld", (long long)pPPS -> cu_qp_delta_enabled_flag));

    if(pPPS -> cu_qp_delta_enabled_flag)
    {
        pitem = AppendItem(pppsItem, "if(cu_qp_delta_enabled_flag)");
        AppendItem(pitem, wxString::Format("diff_cu_qp_delta_depth = %lld", (long long)pPPS -> diff_cu_qp_delta_depth));
    }

    AppendItem(pppsItem, wxString::Format("pps_cb_qp_offset = %lld", (long long)pPPS -> pps_cb_qp_offset));
    AppendItem(pppsItem, wxString::Format("pps_cr_qp_offset = %lld", (long long)pPPS -> pps_cr_qp_offset));
    AppendItem(pppsItem, wxString::Format("pps_slice_chroma_qp_offsets_present_flag = %lld", (long long)pPPS -> pps_slice_chroma_qp_offsets_present_flag));
    AppendItem(pppsItem, wxString::Format("weighted_pred_flag = %lld", (long long)pPPS -> weighted_pred_flag));
    AppendItem(pppsItem, wxString::Format("weighted_bipred_flag = %lld", (long long)pPPS -> weighted_bipred_flag));
    AppendItem(pppsItem, wxString::Format("transquant_bypass_enabled_flag = %lld", (long long)pPPS -> transquant_bypass_enabled_flag));
    AppendItem(pppsItem, wxString::Format("tiles_enabled_flag = %lld", (long long)pPPS -> tiles_enabled_flag));
    AppendItem(pppsItem, wxString::Format("entropy_coding_sync_enabled_flag = %lld", (long long)pPPS -> entropy_coding_sync_enabled_flag));


    if(pPPS -> tiles_enabled_flag)
    {
        pitem = AppendItem(pppsItem, "if(tiles_enabled_flag)");

        AppendItem(pitem, wxString::Format("num_tile_columns_minus1 = %lld", (long long)pPPS -> num_tile_columns_minus1));
        AppendItem(pitem, wxString::Format("num_tile_rows_minus1 = %lld", (long long)pPPS -> num_tile_rows_minus1));

        AppendItem(pitem, wxString::Format("uniform_spacing_flag = %lld", (long long)pPPS -> uniform_spacing_flag));

        if(!pPPS -> uniform_spacing_flag)
        {
            pitemSecond = AppendItem(pitem, "if(!uniform_spacing_flag)");

            wxString str = "column_width_minus1 = {";
            for(int i=0; i<(int)pPPS -> num_tile_columns_minus1 - 1; i++)
            {
                str += wxString::Format("%lld, ", (long long)pPPS -> column_width_minus1[i]);
            }
            if(pPPS -> num_tile_columns_minus1 > 0)
                str += wxString::Format("%lld \n}", (long long)pPPS -> column_width_minus1[pPPS -> num_tile_columns_minus1 - 1]);
            else
                str += "}";

            AppendItem(pitemSecond, str);

            str = "row_height_minus1 = {";
            for(int i=0; i<(int)pPPS -> num_tile_rows_minus1 - 1; i++)
            {
                str += wxString::Format("%lld, ", (long long)pPPS -> row_height_minus1[i]);
            }
            if(pPPS -> num_tile_rows_minus1 > 0)
                str += wxString::Format("%lld \n}", (long long)pPPS -> row_height_minus1[pPPS -> num_tile_rows_minus1 - 1]);
            else
                str += "}";

            AppendItem(pitemSecond, str);
        }

        AppendItem(pitem, wxString::Format("loop_filter_across_tiles_enabled_flag = %lld", (long long)pPPS -> loop_filter_across_tiles_enabled_flag));
    }

    AppendItem(pppsItem, wxString::Format("pps_loop_filter_across_slices_enabled_flag = %lld", (long long)pPPS -> pps_loop_filter_across_slices_enabled_flag));

    AppendItem(pppsItem, wxString::Format("deblocking_filter_control_present_flag = %lld", (long long)pPPS -> deblocking_filter_control_present_flag));

    if(pPPS -> deblocking_filter_control_present_flag)
    {

        pitem = AppendItem(pppsItem, "if(deblocking_filter_control_present_flag)");

        AppendItem(pitem, wxString::Format("deblocking_filter_override_enabled_flag = %lld", (long long)pPPS -> deblocking_filter_override_enabled_flag));

        AppendItem(pitem, wxString::Format("pps_deblocking_filter_disabled_flag = %lld", (long long)pPPS -> pps_deblocking_filter_disabled_flag));

        if(!pPPS -> pps_deblocking_filter_disabled_flag)
        {
            pitemSecond = AppendItem(pitem, "if(!pps_deblocking_filter_disabled_flag)");

            AppendItem(pitemSecond, wxString::Format("pps_beta_offset_div2 = %lld", (long long)pPPS -> pps_beta_offset_div2));
            AppendItem(pitemSecond, wxString::Format("pps_tc_offset_div2 = %lld", (long long)pPPS -> pps_tc_offset_div2));
        }
    }


    AppendItem(pppsItem, wxString::Format("pps_scaling_list_data_present_flag = %lld", (long long)pPPS -> pps_scaling_list_data_present_flag));

    if(pPPS -> pps_scaling_list_data_present_flag)
    {
        pitem = AppendItem(pppsItem, "if(pps_scaling_list_data_present_flag)");

        pitemSecond = AppendItem(pitem, "scaling_list_data");
        createScalingListData(pPPS -> scaling_list_data, pitemSecond);

    }

    AppendItem(pppsItem, wxString::Format("lists_modification_present_flag = %lld", (long long)pPPS -> lists_modification_present_flag));
    AppendItem(pppsItem, wxString::Format("log2_parallel_merge_level_minus2 = %lld", (long long)pPPS -> log2_parallel_merge_level_minus2));
    AppendItem(pppsItem, wxString::Format("slice_segment_header_extension_present_flag = %lld", (long long)pPPS -> slice_segment_header_extension_present_flag));
    AppendItem(pppsItem, wxString::Format("pps_extension_flag = %lld", (long long)pPPS -> pps_extension_flag));
}

void SyntaxViewer::createSlice(std::shared_ptr<HEVC::Slice> pSlice)
{
    wxTreeItemId root = GetRootItem();
    if(pSlice -> m_processFailed) {
        AppendItem(root, "Slice: Process Failed!");
        return;
    }

    std::shared_ptr<HEVC::PPS> pPPS = m_ppsMap[pSlice -> slice_pic_parameter_set_id];
    if(!pPPS) {
        AppendItem(root, wxString::Format("Slice: PPS (ID=%d) not found!", pSlice->slice_pic_parameter_set_id));
        return;
    }

    int32_t spsId = pPPS -> pps_seq_parameter_set_id;
    wxTreeItemId psliceItem;
    wxTreeItemId pitem;
    wxTreeItemId pitemDepend;
    wxTreeItemId pitemThird;
    wxTreeItemId pitemSecond;
    wxTreeItemId pitemLoop;
    wxTreeItemId pStrpc;
    wxTreeItemId pitem1;
    wxTreeItemId pitem2;
    wxTreeItemId pitemRplMod;
    wxTreeItemId pitempwt;

    psliceItem = AppendItem(root, "Slice");

    AppendItem(psliceItem, wxString::Format("first_slice_segment_in_pic_flag = %lld", (long long)pSlice -> first_slice_segment_in_pic_flag));

    if(pSlice -> m_nalHeader.type >= HEVC::NAL_BLA_W_LP && pSlice -> m_nalHeader.type <= HEVC::NAL_IRAP_VCL23)
    {
        pitem = AppendItem(psliceItem, "if ( nal_unit_type >= BLA_W_LP && nal_unit_type <= RSV_IRAP_VCL23)");
        AppendItem(pitem, wxString::Format("no_output_of_prior_pics_flag = %lld", (long long)pSlice -> no_output_of_prior_pics_flag));
    }

    AppendItem(psliceItem, wxString::Format("slice_pic_parameter_set_id = %lld", (long long)pSlice -> slice_pic_parameter_set_id));

    if(!pSlice -> first_slice_segment_in_pic_flag)
    {
        pitem = AppendItem(psliceItem, "if (!first_slice_segment_in_pic_flag)");

        if(pPPS -> dependent_slice_segments_enabled_flag)
        {
            pitemDepend = AppendItem(pitem, wxString::Format("pps -> dependent_slice_segments_enabled_flag = %lld", (long long)pPPS -> dependent_slice_segments_enabled_flag));
            AppendItem(pitemDepend, wxString::Format("dependent_slice_segment_flag = %lld", (long long)pSlice -> dependent_slice_segment_flag));
        }
        AppendItem(pitem, wxString::Format("slice_segment_address = %lld", (long long)pSlice -> slice_segment_address));
    }

    if(!pSlice -> dependent_slice_segment_flag)
    {
        pitem = AppendItem(psliceItem, "if (!dependent_slice_segment_flag)");

        long num_extra_slice_header_bits = pPPS -> num_extra_slice_header_bits;

        wxString str;
        if(num_extra_slice_header_bits > 0)
        {
            str = "slice_reserved_undetermined_flag = {";

            if(num_extra_slice_header_bits > (long)pSlice -> slice_reserved_undetermined_flag.size())
                return;

            for(long i=0; i<num_extra_slice_header_bits - 1; i++)
                str += wxString::Format("%lld, ", (long long)pSlice -> slice_reserved_undetermined_flag[i]);

            str += wxString::Format("%lld, ", (long long)pSlice -> slice_reserved_undetermined_flag[num_extra_slice_header_bits - 1]);
        }
        else
            str = "slice_reserved_undetermined_flag = { }";

        AppendItem(pitem, str);
        AppendItem(pitem, wxString::Format("slice_type = %lld", (long long)pSlice -> slice_type));

        if(pPPS -> output_flag_present_flag)
        {
            pitemDepend = AppendItem(pitem, "if (output_flag_present_flag)");
            AppendItem(pitemDepend, wxString::Format("pic_output_flag = %lld", (long long)pSlice -> pic_output_flag));
        }


        if(!m_spsMap[spsId]) {
            AppendItem(pitem, "ERROR: SPS not found!");
            return;
        }

        if(m_spsMap[spsId] -> separate_colour_plane_flag)
        {
            pitemDepend = AppendItem(pitem, "if (sps -> separate_colour_plane_flag)");
            AppendItem(pitemDepend, wxString::Format("colour_plane_id = %lld", (long long)pSlice -> colour_plane_id));
        }
        bool IdrPicFlag = pSlice -> m_nalHeader.type == HEVC::NAL_IDR_W_RADL || pSlice -> m_nalHeader.type == HEVC::NAL_IDR_N_LP;

        if(!IdrPicFlag)
        {
            pitemDepend = AppendItem(pitem, "if (nal_unit_type != IDR_W_RADL && nal_unit_type != IDR_N_LP)");

            AppendItem(pitemDepend, wxString::Format("slice_pic_order_cnt_lsb = %lld", (long long)pSlice -> slice_pic_order_cnt_lsb));

            pitemThird = AppendItem(pitemDepend, wxString::Format("short_term_ref_pic_set_sps_flag = %lld", (long long)pSlice -> short_term_ref_pic_set_sps_flag));
            if(!pSlice -> short_term_ref_pic_set_sps_flag)
            {
                pitemThird = AppendItem(pitemDepend, "if (!short_term_ref_pic_set_sps_flag)");
                pStrpc = AppendItem(pitemThird, wxString::Format("short_term_ref_pic_set(%lld)", (long long)m_spsMap[spsId] -> num_short_term_ref_pic_sets));
                createShortTermRefPicSet(m_spsMap[spsId] -> num_short_term_ref_pic_sets, pSlice -> short_term_ref_pic_set, m_spsMap[spsId] -> num_short_term_ref_pic_sets, m_spsMap[spsId] -> short_term_ref_pic_set, pStrpc);
            }
            else if(m_spsMap[spsId] -> num_short_term_ref_pic_sets > 1)
            {
                pitemThird = AppendItem(pitemDepend, "if (short_term_ref_pic_set_sps_flag && num_long_term_ref_pics_sps > 0)");
                AppendItem(pitemThird, wxString::Format("short_term_ref_pic_set_idx = %lld", (long long)pSlice -> short_term_ref_pic_set_idx));
            }

            if(m_spsMap[spsId] -> long_term_ref_pics_present_flag)
            {
                pitemThird = AppendItem(pitemDepend, "if (long_term_ref_pics_present_flag)");

                if(m_spsMap[spsId] -> num_long_term_ref_pics_sps > 0)
                {
                    pitemSecond = AppendItem(pitemThird, "if (num_long_term_ref_pics_sps)");
                    AppendItem(pitemSecond, wxString::Format("num_long_term_sps = %lld", (long long)pSlice -> num_long_term_sps));
                }

                AppendItem(pitemThird, wxString::Format("num_long_term_pics = %lld", (long long)pSlice -> num_long_term_pics));

                std::size_t num_long_term = pSlice -> num_long_term_sps + pSlice -> num_long_term_pics;
                pitemLoop = AppendItem(pitemThird, "for( i = 0; i < num_long_term_sps + num_long_term_pics; i++ )");

                for(std::size_t i=0; i < num_long_term; i++)
                {
                    if(i < pSlice -> num_long_term_sps)
                    {
                        pitem1 = AppendItem(pitemLoop, "if (i < num_long_term_sps)");

                        if(m_spsMap[spsId] -> num_long_term_ref_pics_sps > 1)
                        {
                            pitem2 = AppendItem(pitem1, "if (num_long_term_ref_pics_sps > 1)");
                            AppendItem(pitem2, wxString::Format("lt_idx_sps[%lld] = %lld", (long long)i, (long long)pSlice -> lt_idx_sps[i]));
                        }
                    }
                    else
                    {
                        pitem1 = AppendItem(pitemLoop, "if (i >= num_long_term_sps)");
                        AppendItem(pitem1, wxString::Format("poc_lsb_lt[%lld] = %lld", (long long)i, (long long)pSlice -> poc_lsb_lt[i]));
                        AppendItem(pitem1, wxString::Format("used_by_curr_pic_lt_flag[%lld] = %lld", (long long)i, (long long)pSlice -> used_by_curr_pic_lt_flag[i]));
                    }

                    AppendItem(pitemLoop, wxString::Format("delta_poc_msb_present_flag[%lld] = %lld", (long long)i, (long long)pSlice -> delta_poc_msb_present_flag[i]));
                    if(pSlice -> delta_poc_msb_present_flag[i])
                    {
                        pitem1 = AppendItem(pitemLoop, wxString::Format("if (delta_poc_msb_present_flag[%lld])", (long long)i));
                        AppendItem(pitem1, wxString::Format("delta_poc_msb_cycle_lt[%lld] = %lld", (long long)i, (long long)pSlice -> delta_poc_msb_cycle_lt[i]));
                    }
                }
            }

            if(m_spsMap[spsId] -> sps_temporal_mvp_enabled_flag)
            {
                pitemSecond = AppendItem(pitemDepend, "if (sps_temporal_mvp_enabled_flag)");
                AppendItem(pitemSecond, wxString::Format("slice_temporal_mvp_enabled_flag = %lld", (long long)pSlice -> slice_temporal_mvp_enabled_flag));
            }
        }

        if(m_spsMap[spsId] -> sample_adaptive_offset_enabled_flag)
        {
            pitemDepend = AppendItem(pitem, "if (sample_adaptive_offset_enabled_flag)");

            AppendItem(pitemDepend, wxString::Format("slice_sao_luma_flag = %lld", (long long)pSlice -> slice_sao_luma_flag));
            AppendItem(pitemDepend, wxString::Format("slice_sao_chroma_flag = %lld", (long long)pSlice -> slice_sao_chroma_flag));
        }

        if(pSlice -> slice_type == SLICE_B || pSlice -> slice_type == SLICE_P)
        {
            pitemDepend = AppendItem(pitem, "if (slice_type == P || slice_type == B)");

            AppendItem(pitemDepend, wxString::Format("num_ref_idx_active_override_flag = %lld", (long long)pSlice -> num_ref_idx_active_override_flag));

            if(pSlice -> num_ref_idx_active_override_flag)
            {
                pitemSecond = AppendItem(pitemDepend, "if (num_ref_idx_active_override_flag)");

                AppendItem(pitemSecond, wxString::Format("num_ref_idx_l0_active_minus1 = %lld", (long long)pSlice -> num_ref_idx_l0_active_minus1));

                if(pSlice -> slice_type == SLICE_B)
                {
                    pitemThird = AppendItem(pitemSecond, "if (slice_type == B)");
                    AppendItem(pitemThird, wxString::Format("num_ref_idx_l1_active_minus1 = %lld", (long long)pSlice -> num_ref_idx_l1_active_minus1));
                }
            }

            if(pPPS -> lists_modification_present_flag)
            {
                std::size_t NumPocTotalCurr = HEVC::calcNumPocTotalCurr(pSlice, m_spsMap[spsId]);

                if(NumPocTotalCurr > 1)
                {
                    pitemThird = AppendItem(pitemDepend, "if (lists_modification_present_flag && NumPocTotalCurr > 1 )");
                    pitemRplMod = AppendItem(pitemThird, wxString::Format("short_term_ref_pic_set(%lld)", (long long)m_spsMap[spsId] -> num_short_term_ref_pic_sets));


                    createRefPicListModification(pSlice -> ref_pic_lists_modification, pitemRplMod);
                }
            }

            if(pSlice -> slice_type == SLICE_B)
            {
                pitemThird = AppendItem(pitemDepend, "if (slice_type == B)");
                AppendItem(pitemThird, wxString::Format("mvd_l1_zero_flag = %lld", (long long)pSlice -> mvd_l1_zero_flag));
            }

            if(pPPS -> cabac_init_present_flag)
            {
                pitemThird = AppendItem(pitemDepend, "if (cabac_init_present_flag)");
                AppendItem(pitemThird, wxString::Format("cabac_init_flag = %lld", (long long)pSlice -> cabac_init_flag));
            }

            if(pSlice -> slice_temporal_mvp_enabled_flag)
            {
                pitemThird = AppendItem(pitemDepend, "if (slice_temporal_mvp_enabled_flag)");

                if(pSlice -> slice_type == SLICE_B)
                {
                    pitemSecond = AppendItem(pitemThird, "if (slice_type == B)");
                    AppendItem(pitemSecond, wxString::Format("collocated_from_l0_flag = %lld", (long long)pSlice -> collocated_from_l0_flag));
                }

                if((pSlice -> collocated_from_l0_flag && pSlice -> num_ref_idx_l0_active_minus1) ||
                    (!pSlice -> collocated_from_l0_flag && pSlice -> num_ref_idx_l1_active_minus1))
                {
                    pitemSecond = AppendItem(pitemThird, "if ((collocated_from_l0_flag && num_ref_idx_l0_active_minus1 > 0 ) || (!collocated_from_l0_flag && num_ref_idx_l1_active_minus1 > 0))");

                    AppendItem(pitemSecond, wxString::Format("collocated_ref_idx = %lld", (long long)pSlice -> collocated_ref_idx));
                }
            }

            if((pPPS -> weighted_pred_flag && pSlice -> slice_type == SLICE_P) ||
                (pPPS -> weighted_bipred_flag && pSlice -> slice_type == SLICE_B))
            {
                pitemThird = AppendItem(pitemDepend, "if ((weighted_pred_flag && slice_type == P) || (weighted_bipred_flag && slice_type == B))");

                pitempwt = AppendItem(pitemThird, "pred_weight_table");

                createPredWeightTable(pSlice -> pred_weight_table, pSlice, pitempwt);
            }

            AppendItem(pitemDepend, wxString::Format("five_minus_max_num_merge_cand = %lld", (long long)pSlice -> five_minus_max_num_merge_cand));
        }

        AppendItem(pitem, wxString::Format("slice_qp_delta = %lld", (long long)pSlice -> slice_qp_delta));


        if(pPPS -> pps_slice_chroma_qp_offsets_present_flag)
        {
            pitemDepend = AppendItem(pitem, "if (pps_slice_chroma_qp_offsets_present_flag)");

            AppendItem(pitemDepend, wxString::Format("slice_cb_qp_offset = %lld", (long long)pSlice -> slice_cb_qp_offset));
            AppendItem(pitemDepend, wxString::Format("slice_cr_qp_offset = %lld", (long long)pSlice -> slice_cr_qp_offset));
        }

        if(pPPS -> deblocking_filter_override_enabled_flag)
        {
            pitemDepend = AppendItem(pitem, "if (deblocking_filter_override_enabled_flag)");
            AppendItem(pitemDepend, wxString::Format("deblocking_filter_override_flag = %lld", (long long)pSlice -> deblocking_filter_override_flag));
        }

        if(pSlice -> deblocking_filter_override_flag)
        {
            pitemDepend = AppendItem(pitem, "if (deblocking_filter_override_flag)");

            AppendItem(pitemDepend, wxString::Format("slice_deblocking_filter_disabled_flag = %lld", (long long)pSlice -> slice_deblocking_filter_disabled_flag));

            if(!pSlice -> slice_deblocking_filter_disabled_flag)
            {
                pitemThird = AppendItem(pitemDepend, "if (!slice_deblocking_filter_disabled_flag)");

                AppendItem(pitemThird, wxString::Format("slice_beta_offset_div2 = %lld", (long long)pSlice -> slice_beta_offset_div2));
                AppendItem(pitemThird, wxString::Format("slice_tc_offset_div2 = %lld", (long long)pSlice -> slice_tc_offset_div2));
            }
        }

        if(pPPS -> pps_loop_filter_across_slices_enabled_flag &&
            (pSlice -> slice_sao_luma_flag || pSlice -> slice_sao_chroma_flag || !pSlice -> slice_deblocking_filter_disabled_flag))
        {
            pitemDepend = AppendItem(pitem, "if (pps_loop_filter_across_slices_enabled_flag && (slice_sao_luma_flag || slice_sao_chroma_flag || !slice_deblocking_filter_disabled_flag ))");

            AppendItem(pitemDepend, wxString::Format("slice_loop_filter_across_slices_enabled_flag = %lld", (long long)pSlice -> slice_loop_filter_across_slices_enabled_flag));
        }
    }

    if(pPPS -> tiles_enabled_flag || pPPS -> entropy_coding_sync_enabled_flag)
    {
        pitem = AppendItem(psliceItem, "if (tiles_enabled_flag || entropy_coding_sync_enabled_flag)");

        AppendItem(pitem, wxString::Format("num_entry_point_offsets = %lld", (long long)pSlice -> num_entry_point_offsets));

        if(pSlice -> num_entry_point_offsets > 0)
        {
            pitemThird = AppendItem(pitem, "if (num_entry_point_offsets > 0)");

            AppendItem(pitemThird, wxString::Format("offset_len_minus1 = %lld", (long long)pSlice -> offset_len_minus1));

            pitemLoop = AppendItem(pitemThird, "for( i = 0; i < num_entry_point_offsets; i++ )");

            for(std::size_t i=0; i<pSlice -> num_entry_point_offsets; i++)
                AppendItem(pitemLoop, wxString::Format("entry_point_offset_minus1[%lld] = %lld", (long long)i, (long long)pSlice -> entry_point_offset_minus1[i]));
        }
    }

    if(pPPS -> slice_segment_header_extension_present_flag)
    {
        pitem = AppendItem(psliceItem, "if (slice_segment_header_extension_present_flag)");

        AppendItem(pitem, wxString::Format("slice_segment_header_extension_length = %lld", (long long)pSlice -> slice_segment_header_extension_length));

        pitemLoop = AppendItem(pitem, "for( i = 0; i < slice_segment_header_extension_length; i++ )");

        for(std::size_t i=0; i<pSlice -> slice_segment_header_extension_length; i++)
        {
            AppendItem(pitemLoop, wxString::Format("slice_segment_header_extension_data_byte[%lld] = %lld", (long long)i, (long long)pSlice -> slice_segment_header_extension_data_byte[i]));
        }
    }
}

void SyntaxViewer::createAUD(std::shared_ptr<HEVC::AUD> pAUD)
{
    wxTreeItemId root = GetRootItem();
    wxTreeItemId pitem = AppendItem(root, "AUD");
    AppendItem(pitem, wxString::Format("pic_type = %lld", (long long)pAUD -> pic_type));
}


void SyntaxViewer::createSEI(std::shared_ptr<HEVC::SEI> pSEI)
{
    wxTreeItemId root = GetRootItem();
    wxTreeItemId pitemSEI;
    wxTreeItemId pitem;
    wxTreeItemId pitemSecond;
    wxTreeItemId pitemDecPictHash;
    wxTreeItemId pitemSei;

    pitemSEI = AppendItem(root, "SEI");

    for(std::size_t i=0; i< pSEI -> sei_message.size(); i++)
    {
        std::size_t payloadType = 0;
        std::size_t payloadSize = 0;

        pitem = AppendItem(pitemSEI, wxString::Format("sei_message(%lld)", (long long)i));

        if(pSEI -> sei_message[i].num_payload_type_ff_bytes)
        {
            pitemSecond = AppendItem(pitem, "while( next_bits( 8 ) == 0xFF )");
            for(std::size_t j=0; j<pSEI -> sei_message[i].num_payload_type_ff_bytes; j++)
            {
                AppendItem(pitemSecond, "0xFF");
                payloadType += 255;
            }
        }

        AppendItem(pitem, wxString::Format("last_payload_type_byte = %lld", (long long)pSEI -> sei_message[i].last_payload_type_byte));
        payloadType += pSEI -> sei_message[i].last_payload_type_byte;


        if(pSEI -> sei_message[i].num_payload_size_ff_bytes)
        {
            pitemSecond = AppendItem(pitem, "while( next_bits( 8 ) == 0xFF )");
            for(std::size_t j=0; j<pSEI -> sei_message[i].num_payload_size_ff_bytes; j++)
            {
                AppendItem(pitemSecond, "0xFF");
                payloadSize += 255;
            }
        }

        AppendItem(pitem, wxString::Format("last_payload_size_byte = %lld", (long long)pSEI -> sei_message[i].last_payload_size_byte));

        payloadSize += pSEI -> sei_message[i].last_payload_size_byte;
        switch(payloadType)
        {
            case HEVC::SeiMessage::DECODED_PICTURE_HASH:
            {
                std::shared_ptr<HEVC::DecodedPictureHash> pdecPictHash = std::dynamic_pointer_cast<HEVC::DecodedPictureHash>(pSEI -> sei_message[i].sei_payload);

                pitemDecPictHash = AppendItem(pitem, wxString::Format("decoded_picture_hash(%lld)", (long long)payloadSize));
                createDecodedPictureHash(pdecPictHash, pitemDecPictHash);
                break;
            }

            case HEVC::SeiMessage::USER_DATA_UNREGISTERED:
            {
                std::shared_ptr<HEVC::UserDataUnregistered> pSeiMessage = std::dynamic_pointer_cast<HEVC::UserDataUnregistered>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("user_data_unregistered(%lld)", (long long)payloadSize));
                createUserDataUnregistered(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::SCENE_INFO:
            {
                std::shared_ptr<HEVC::SceneInfo> pSeiMessage = std::dynamic_pointer_cast<HEVC::SceneInfo>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("scene_info(%lld)", (long long)payloadSize));
                createSceneInfo(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::FULL_FRAME_SNAPSHOT:
            {
                std::shared_ptr<HEVC::FullFrameSnapshot> pSeiMessage = std::dynamic_pointer_cast<HEVC::FullFrameSnapshot>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("picture_snapshot(%lld)", (long long)payloadSize));
                createFullFrameSnapshot(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::PROGRESSIVE_REFINEMENT_SEGMENT_START:
            {
                std::shared_ptr<HEVC::ProgressiveRefinementSegmentStart> pSeiMessage = std::dynamic_pointer_cast<HEVC::ProgressiveRefinementSegmentStart>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("progressive_refinement_segment_start(%lld)", (long long)payloadSize));
                createProgressiveRefinementSegmentStart(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::PROGRESSIVE_REFINEMENT_SEGMENT_END:
            {
                std::shared_ptr<HEVC::ProgressiveRefinementSegmentEnd> pSeiMessage = std::dynamic_pointer_cast<HEVC::ProgressiveRefinementSegmentEnd>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("progressive_refinement_segment_end(%lld)", (long long)payloadSize));
                createProgressiveRefinementSegmentEnd(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::BUFFERING_PERIOD:
            {
                std::shared_ptr<HEVC::BufferingPeriod> pSeiMessage = std::dynamic_pointer_cast<HEVC::BufferingPeriod>(pSEI -> sei_message[i].sei_payload);
                pitemSei = AppendItem(pitem, wxString::Format("buffering_period(%lld)", (long long)payloadSize));
                createBufferingPeriod(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::FILLER_PAYLOAD:
            {
                pitemSei = AppendItem(pitem, wxString::Format("filler_payload(%lld)", (long long)payloadSize));
                break;
            }

            case HEVC::SeiMessage::PICTURE_TIMING:
            {
                std::shared_ptr<HEVC::PicTiming> pSeiMessage = std::dynamic_pointer_cast<HEVC::PicTiming>(pSEI -> sei_message[i].sei_payload);
                pitemSei = AppendItem(pitem, wxString::Format("pic_timing(%lld)", (long long)payloadSize));
                createPicTiming(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::RECOVERY_POINT:
            {
                std::shared_ptr<HEVC::RecoveryPoint> pSeiMessage = std::dynamic_pointer_cast<HEVC::RecoveryPoint>(pSEI -> sei_message[i].sei_payload);
                pitemSei = AppendItem(pitem, wxString::Format("recovery_point(%lld)", (long long)payloadSize));
                createRecoveryPoint(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::TONE_MAPPING_INFO:
            {
                std::shared_ptr<HEVC::ToneMapping> pSeiMessage = std::dynamic_pointer_cast<HEVC::ToneMapping>(pSEI -> sei_message[i].sei_payload);
                pitemSei = AppendItem(pitem, wxString::Format("tone_mapping_info(%lld)", (long long)payloadSize));
                createToneMapping(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::FRAME_PACKING:
            {
                std::shared_ptr<HEVC::FramePacking> pSeiMessage = std::dynamic_pointer_cast<HEVC::FramePacking>(pSEI -> sei_message[i].sei_payload);
                pitemSei = AppendItem(pitem, wxString::Format("frame_packing_arrangement(%lld)", (long long)payloadSize));
                createFramePacking(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::DISPLAY_ORIENTATION:
            {
                std::shared_ptr<HEVC::DisplayOrientation> pSeiMessage = std::dynamic_pointer_cast<HEVC::DisplayOrientation>(pSEI -> sei_message[i].sei_payload);
                pitemSei = AppendItem(pitem, wxString::Format("display_orientation(%lld)", (long long)payloadSize));
                createDisplayOrientation(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::SOP_DESCRIPTION:
            {
                std::shared_ptr<HEVC::SOPDescription> pSeiMessage = std::dynamic_pointer_cast<HEVC::SOPDescription>(pSEI -> sei_message[i].sei_payload);
                pitemSei = AppendItem(pitem, wxString::Format("structure_of_pictures_info(%lld)", (long long)payloadSize));
                createSOPDescription(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::ACTIVE_PARAMETER_SETS:
            {
                std::shared_ptr<HEVC::ActiveParameterSets> pSeiMessage = std::dynamic_pointer_cast<HEVC::ActiveParameterSets>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("active_parameter_sets(%lld)", (long long)payloadSize));
                createActiveParameterSets(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::TEMPORAL_LEVEL0_INDEX:
            {
                std::shared_ptr<HEVC::TemporalLevel0Index> pSeiMessage = std::dynamic_pointer_cast<HEVC::TemporalLevel0Index>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("temporal_sub_layer_zero_index(%lld)", (long long)payloadSize));
                createTemporalLevel0Index(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::REGION_REFRESH_INFO:
            {
                std::shared_ptr<HEVC::RegionRefreshInfo> pSeiMessage = std::dynamic_pointer_cast<HEVC::RegionRefreshInfo>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("region_refresh_info(%lld)", (long long)payloadSize));
                createRegionRefreshInfo(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::TIME_CODE:
            {
                std::shared_ptr<HEVC::TimeCode> pSeiMessage = std::dynamic_pointer_cast<HEVC::TimeCode>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("time_code(%lld)", (long long)payloadSize));
                createTimeCode(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::MASTERING_DISPLAY_INFO:
            {
                std::shared_ptr<HEVC::MasteringDisplayInfo> pSeiMessage = std::dynamic_pointer_cast<HEVC::MasteringDisplayInfo>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("mastering_display_info(%lld)", (long long)payloadSize));
                createMasteringDisplayInfo(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::SEGM_RECT_FRAME_PACKING:
            {
                std::shared_ptr<HEVC::SegmRectFramePacking> pSeiMessage = std::dynamic_pointer_cast<HEVC::SegmRectFramePacking>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("segm_rect_frame_packing(%lld)", (long long)payloadSize));
                createSegmRectFramePacking(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::KNEE_FUNCTION_INFO:
            {
                std::shared_ptr<HEVC::KneeFunctionInfo> pSeiMessage = std::dynamic_pointer_cast<HEVC::KneeFunctionInfo>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("knee_function_info(%lld)", (long long)payloadSize));
                createKneeFunctionInfo(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::CHROMA_RESAMPLING_FILTER_HINT:
            {
                std::shared_ptr<HEVC::ChromaResamplingFilterHint> pSeiMessage = std::dynamic_pointer_cast<HEVC::ChromaResamplingFilterHint>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("chroma_resampling_filter_hint(%lld)", (long long)payloadSize));
                createChromaResamplingFilterHint(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::COLOUR_REMAPPING_INFO:
            {
                std::shared_ptr<HEVC::ColourRemappingInfo> pSeiMessage = std::dynamic_pointer_cast<HEVC::ColourRemappingInfo>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("colour_remapping_info(%lld)", (long long)payloadSize));
                createColourRemappingInfo(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::CONTENT_LIGHT_LEVEL_INFO:
            {
                std::shared_ptr<HEVC::ContentLightLevelInfo> pSeiMessage = std::dynamic_pointer_cast<HEVC::ContentLightLevelInfo>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("content_light_level_info(%lld)", (long long)payloadSize));
                createContentLightLevelInfo(pSeiMessage, pitemSei);
                break;
            }

            case HEVC::SeiMessage::ALTERNATIVE_TRANSFER_CHARACTERISTICS:
            {
                std::shared_ptr<HEVC::AlternativeTransferCharacteristics> pSeiMessage = std::dynamic_pointer_cast<HEVC::AlternativeTransferCharacteristics>(pSEI -> sei_message[i].sei_payload);

                pitemSei = AppendItem(pitem, wxString::Format("alternative_transfer_characteristics(%lld)", (long long)payloadSize));
                createAlternativeTransferCharacteristics(pSeiMessage, pitemSei);
                break;
            }

            default:
                AppendItem(pitem, wxString::Format("sei_payload(%lld, %lld)", (long long)payloadType, (long long)payloadSize));
        }
    }
}


void SyntaxViewer::createProfileTierLevel(const HEVC::ProfileTierLevel &ptl, wxTreeItemId pPtlItem)
{
    wxTreeItemId pitem;
    wxTreeItemId pitemLoop;
    wxString str;

    AppendItem(pPtlItem, wxString::Format("general_profile_space = %lld", (long long)ptl.general_profile_space));
    AppendItem(pPtlItem, wxString::Format("general_tier_flag = %lld", (long long)ptl.general_tier_flag));
    AppendItem(pPtlItem, wxString::Format("general_profile_idc = %lld", (long long)ptl.general_profile_idc));

    str = "general_profile_compatibility_flag[i] = {";
    for(std::size_t i=0; i<31; i++)
    {
        str += wxString::Format("%lld, ", (long long)ptl.general_profile_compatibility_flag[i]);
    }
    str += wxString::Format("%lld\n}", (long long)ptl.general_profile_compatibility_flag[31]);
    AppendItem(pPtlItem, str);

    AppendItem(pPtlItem, wxString::Format("general_progressive_source_flag = %lld", (long long)ptl.general_progressive_source_flag));
    AppendItem(pPtlItem, wxString::Format("general_interlaced_source_flag = %lld", (long long)ptl.general_interlaced_source_flag));
    AppendItem(pPtlItem, wxString::Format("general_non_packed_constraint_flag = %lld", (long long)ptl.general_non_packed_constraint_flag));
    AppendItem(pPtlItem, wxString::Format("general_frame_only_constraint_flag = %lld", (long long)ptl.general_frame_only_constraint_flag));
    AppendItem(pPtlItem, wxString::Format("general_level_idc = %lld", (long long)ptl.general_level_idc));

    if(ptl.sub_layer_profile_present_flag.size() == 0)
    {
        AppendItem(pPtlItem, "sub_layer_profile_present_flag = {}");
        AppendItem(pPtlItem, "sub_layer_level_present_flag = {}");
    }
    else
    {
        str = "sub_layer_profile_present_flag = {";
        for(std::size_t i=0; i<ptl.sub_layer_profile_present_flag.size() - 1; i++)
        {
            str += wxString::Format("%lld, ", (long long)ptl.sub_layer_profile_present_flag[i]);
        }
        str += wxString::Format("%lld \n}", (long long)ptl.sub_layer_profile_present_flag[ptl.sub_layer_profile_present_flag.size() - 1]);
        AppendItem(pPtlItem, str);

        str = "sub_layer_level_present_flag = {";
        for(std::size_t i=0; i<ptl.sub_layer_profile_present_flag.size() - 1; i++)
        {
            str += wxString::Format("%lld, ", (long long)ptl.sub_layer_level_present_flag[i]);
        }
        str += wxString::Format("%lld \n}", (long long)ptl.sub_layer_level_present_flag[ptl.sub_layer_profile_present_flag.size() - 1]);
        AppendItem(pPtlItem, str);
    }

    std::size_t maxNumSubLayersMinus1 = ptl.sub_layer_profile_present_flag.size();
    bool needLoop = false;

    for(std::size_t i=0; i<maxNumSubLayersMinus1 && !needLoop; i++)
    {
        if(ptl.sub_layer_profile_present_flag[i])
            needLoop = true;

        if(ptl.sub_layer_level_present_flag[i])
            needLoop = true;
    }

    if(needLoop)
    {
        pitemLoop = AppendItem(pPtlItem, "for( i = 0; i < maxNumSubLayersMinus1; i++ )");

        for(std::size_t i=0; i<maxNumSubLayersMinus1; i++)
        {
            if(ptl.sub_layer_profile_present_flag[i])
            {
                pitem = AppendItem(pitemLoop, wxString::Format("if( sub_layer_profile_present_flag[%lld] )", (long long)i));


                AppendItem(pitem, wxString::Format("sub_layer_profile_space[%lld] = %lld", (long long)i, (long long)ptl.sub_layer_profile_space[i]));
                AppendItem(pitem, wxString::Format("sub_layer_tier_flag[%lld] = %lld", (long long)i, (long long)ptl.sub_layer_tier_flag[i]));
                AppendItem(pitem, wxString::Format("sub_layer_profile_idc[%lld] = %lld", (long long)i, (long long)ptl.sub_layer_profile_idc[i]));

                str = "sub_layer_profile_compatibility_flag = { ";
                for(std::size_t j=0; j<31; j++)
                {
                    str += wxString::Format("%lld, ", (long long)ptl.sub_layer_profile_compatibility_flag[i][j]);
                }
                str += wxString::Format("%lld \n}", (long long)ptl.sub_layer_profile_compatibility_flag[i][31]);
                AppendItem(pitem, str);


                AppendItem(pitem, wxString::Format("sub_layer_progressive_source_flag[%lld] = %lld", (long long)i, (long long)ptl.sub_layer_progressive_source_flag[i]));
                AppendItem(pitem, wxString::Format("sub_layer_interlaced_source_flag[%lld] = %lld", (long long)i, (long long)ptl.sub_layer_interlaced_source_flag[i]));
                AppendItem(pitem, wxString::Format("sub_layer_non_packed_constraint_flag[%lld] = %lld", (long long)i, (long long)ptl.sub_layer_non_packed_constraint_flag[i]));
                AppendItem(pitem, wxString::Format("sub_layer_frame_only_constraint_flag[%lld] = %lld", (long long)i, (long long)ptl.sub_layer_frame_only_constraint_flag[i]));
            }

            if(ptl.sub_layer_level_present_flag[i])
            {
                pitem = AppendItem(pitemLoop, wxString::Format("if( sub_layer_level_present_flag[%lld] )", (long long)i));
                AppendItem(pitem, wxString::Format("sub_layer_level_idc[%lld] = %lld", (long long)i, (long long)ptl.sub_layer_level_idc[i]));
            }
        }
    }
}


void SyntaxViewer::createVuiParameters(const HEVC::VuiParameters &vui, std::size_t maxNumSubLayersMinus1, wxTreeItemId pVuiItem)
{
    wxTreeItemId pitem;
    wxTreeItemId pitemSecond;
    wxTreeItemId pitemHrd;

    AppendItem(pVuiItem, wxString::Format("aspect_ratio_info_present_flag = %lld", (long long)vui.aspect_ratio_info_present_flag));

    if(vui.aspect_ratio_info_present_flag)
    {
        pitem = AppendItem(pVuiItem, "if( aspect_ratio_info_present_flag )");

        AppendItem(pitem, wxString::Format("aspect_ratio_idc = %lld", (long long)vui.aspect_ratio_idc));

        if(vui.aspect_ratio_idc == 255) //EXTENDED_SAR
        {
            pitemSecond = AppendItem(pitem, "if( aspect_ratio_idc == EXTENDED_SAR )");

            AppendItem(pitemSecond, wxString::Format("sar_width = %lld", (long long)vui.sar_width));
            AppendItem(pitemSecond, wxString::Format("sar_height = %lld", (long long)vui.sar_height));
        }
    }

    AppendItem(pVuiItem, wxString::Format("overscan_info_present_flag = %lld", (long long)vui.overscan_info_present_flag));

    if(vui.overscan_info_present_flag)
    {
        pitem = AppendItem(pVuiItem, "if( overscan_info_present_flag )");
        AppendItem(pitem, wxString::Format("overscan_appropriate_flag = %lld", (long long)vui.overscan_appropriate_flag));
    }

    AppendItem(pVuiItem, wxString::Format("video_signal_type_present_flag = %lld", (long long)vui.video_signal_type_present_flag));

    if(vui.video_signal_type_present_flag)
    {
        pitem = AppendItem(pVuiItem, "if( video_signal_type_present_flag )");

        AppendItem(pitem, wxString::Format("video_format = %lld", (long long)vui.video_format));
        AppendItem(pitem, wxString::Format("video_full_range_flag = %lld", (long long)vui.video_full_range_flag));
        AppendItem(pitem, wxString::Format("colour_description_present_flag = %lld", (long long)vui.colour_description_present_flag));

        if(vui.colour_description_present_flag)
        {
            pitemSecond = AppendItem(pitem, "if( colour_description_present_flag )");

            AppendItem(pitemSecond, wxString::Format("colour_primaries = %lld", (long long)vui.colour_primaries));
            AppendItem(pitemSecond, wxString::Format("transfer_characteristics = %lld", (long long)vui.transfer_characteristics));
            AppendItem(pitemSecond, wxString::Format("matrix_coeffs = %lld", (long long)vui.matrix_coeffs));
        }
    }

    AppendItem(pVuiItem, wxString::Format("chroma_loc_info_present_flag = %lld", (long long)vui.chroma_loc_info_present_flag));

    if(vui.chroma_loc_info_present_flag)
    {
        pitem = AppendItem(pVuiItem, "if( chroma_loc_info_present_flag )");

        AppendItem(pitem, wxString::Format("chroma_sample_loc_type_top_field = %lld", (long long)vui.chroma_sample_loc_type_top_field));
        AppendItem(pitem, wxString::Format("chroma_sample_loc_type_bottom_field = %lld", (long long)vui.chroma_sample_loc_type_bottom_field));
    }

    AppendItem(pVuiItem, wxString::Format("neutral_chroma_indication_flag = %lld", (long long)vui.neutral_chroma_indication_flag));
    AppendItem(pVuiItem, wxString::Format("field_seq_flag = %lld", (long long)vui.field_seq_flag));
    AppendItem(pVuiItem, wxString::Format("frame_field_info_present_flag = %lld", (long long)vui.frame_field_info_present_flag));
    AppendItem(pVuiItem, wxString::Format("default_display_window_flag = %lld", (long long)vui.default_display_window_flag));


    if(vui.default_display_window_flag)
    {
        pitem = AppendItem(pVuiItem, "if( default_display_window_flag )");

        AppendItem(pitem, wxString::Format("def_disp_win_left_offset = %lld", (long long)vui.def_disp_win_left_offset));
        AppendItem(pitem, wxString::Format("def_disp_win_right_offset = %lld", (long long)vui.def_disp_win_right_offset));
        AppendItem(pitem, wxString::Format("def_disp_win_top_offset = %lld", (long long)vui.def_disp_win_top_offset));
        AppendItem(pitem, wxString::Format("def_disp_win_bottom_offset = %lld", (long long)vui.def_disp_win_bottom_offset));
    }

    AppendItem(pVuiItem, wxString::Format("vui_timing_info_present_flag = %lld", (long long)vui.vui_timing_info_present_flag));

    if(vui.vui_timing_info_present_flag)
    {
        pitem = AppendItem(pVuiItem, "if( vui_timing_info_present_flag )");

        AppendItem(pitem, wxString::Format("vui_num_units_in_tick = %lld", (long long)vui.vui_num_units_in_tick));
        AppendItem(pitem, wxString::Format("vui_time_scale = %lld", (long long)vui.vui_time_scale));
        AppendItem(pitem, wxString::Format("vui_poc_proportional_to_timing_flag = %lld", (long long)vui.vui_poc_proportional_to_timing_flag));

        if(vui.vui_poc_proportional_to_timing_flag)
        {
            pitemSecond = AppendItem(pitem, "if( vui_poc_proportional_to_timing_flag )");
            AppendItem(pitemSecond, wxString::Format("vui_num_ticks_poc_diff_one_minus1 = %lld", (long long)vui.vui_num_ticks_poc_diff_one_minus1));

            AppendItem(pitemSecond, wxString::Format("vui_hrd_parameters_present_flag = %lld", (long long)vui.vui_hrd_parameters_present_flag));
        }

        AppendItem(pitem, wxString::Format("vui_hrd_parameters_present_flag = %lld", (long long)vui.vui_hrd_parameters_present_flag));

        if(vui.vui_hrd_parameters_present_flag)
        {
            pitemSecond = AppendItem(pitem, "if( vui_hrd_parameters_present_flag )");

            pitemHrd = AppendItem(pitemSecond, wxString::Format("hrd_parameters(1, %lld)", (long long)maxNumSubLayersMinus1));

            createHrdParameters(vui.hrd_parameters, 1, pitemHrd);
        }
    }

    AppendItem(pVuiItem, wxString::Format("bitstream_restriction_flag = %lld", (long long)vui.bitstream_restriction_flag));

    if(vui.bitstream_restriction_flag)
    {
        pitem = AppendItem(pVuiItem, "if( bitstream_restriction_flag )");

        AppendItem(pitem, wxString::Format("tiles_fixed_structure_flag = %lld", (long long)vui.tiles_fixed_structure_flag));
        AppendItem(pitem, wxString::Format("motion_vectors_over_pic_boundaries_flag = %lld", (long long)vui.motion_vectors_over_pic_boundaries_flag));
        AppendItem(pitem, wxString::Format("restricted_ref_pic_lists_flag = %lld", (long long)vui.restricted_ref_pic_lists_flag));
        AppendItem(pitem, wxString::Format("min_spatial_segmentation_idc = %lld", (long long)vui.min_spatial_segmentation_idc));
        AppendItem(pitem, wxString::Format("max_bytes_per_pic_denom = %lld", (long long)vui.max_bytes_per_pic_denom));
        AppendItem(pitem, wxString::Format("max_bits_per_min_cu_denom = %lld", (long long)vui.max_bits_per_min_cu_denom));
        AppendItem(pitem, wxString::Format("log2_max_mv_length_horizontal = %lld", (long long)vui.log2_max_mv_length_horizontal));
        AppendItem(pitem, wxString::Format("log2_max_mv_length_vertical = %lld", (long long)vui.log2_max_mv_length_vertical));
    }
}


void SyntaxViewer::createHrdParameters(const HEVC::HrdParameters &hrd, uint8_t commonInfPresentFlag, wxTreeItemId pHrdItem)
{
    wxTreeItemId pitem;
    wxTreeItemId pitemSecond;
    wxTreeItemId pitemThird;

    if(commonInfPresentFlag)
    {
        pitem = AppendItem(pHrdItem, "if( commonInfPresentFlag )");

        AppendItem(pitem, wxString::Format("nal_hrd_parameters_present_flag = %lld", (long long)hrd.nal_hrd_parameters_present_flag));
        AppendItem(pitem, wxString::Format("vcl_hrd_parameters_present_flag = %lld", (long long)hrd.vcl_hrd_parameters_present_flag));

        if(hrd.nal_hrd_parameters_present_flag || hrd.vcl_hrd_parameters_present_flag)
        {
            pitemSecond = AppendItem(pitem, "if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag )");

            AppendItem(pitemSecond, wxString::Format("sub_pic_hrd_params_present_flag = %lld", (long long)hrd.sub_pic_hrd_params_present_flag));
            if(hrd.sub_pic_hrd_params_present_flag)
            {
                pitemThird = AppendItem(pitemSecond, "if (sub_pic_hrd_params_present_flag )");

                AppendItem(pitemThird, wxString::Format("tick_divisor_minus2 = %lld", (long long)hrd.tick_divisor_minus2));
                AppendItem(pitemThird, wxString::Format("du_cpb_removal_delay_increment_length_minus1 = %lld", (long long)hrd.du_cpb_removal_delay_increment_length_minus1));
                AppendItem(pitemThird, wxString::Format("sub_pic_cpb_params_in_pic_timing_sei_flag = %lld", (long long)hrd.sub_pic_cpb_params_in_pic_timing_sei_flag));
                AppendItem(pitemThird, wxString::Format("dpb_output_delay_du_length_minus1 = %lld", (long long)hrd.dpb_output_delay_du_length_minus1));
            }

            AppendItem(pitemSecond, wxString::Format("bit_rate_scale = %lld", (long long)hrd.bit_rate_scale));
            AppendItem(pitemSecond, wxString::Format("cpb_size_scale = %lld", (long long)hrd.cpb_size_scale));

            if(hrd.sub_pic_hrd_params_present_flag)
            {
                pitemThird = AppendItem(pitemSecond, "if (sub_pic_hrd_params_present_flag )");

                AppendItem(pitemThird, wxString::Format("cpb_size_du_scale = %lld", (long long)hrd.cpb_size_du_scale));
            }

            AppendItem(pitemSecond, wxString::Format("initial_cpb_removal_delay_length_minus1 = %lld", (long long)hrd.initial_cpb_removal_delay_length_minus1));
            AppendItem(pitemSecond, wxString::Format("au_cpb_removal_delay_length_minus1 = %lld", (long long)hrd.au_cpb_removal_delay_length_minus1));
            AppendItem(pitemSecond, wxString::Format("dpb_output_delay_length_minus1 = %lld", (long long)hrd.dpb_output_delay_length_minus1));
        }
    }

    if(hrd.fixed_pic_rate_general_flag.size() > 0)
    {
        pitem = AppendItem(pHrdItem, "for( i = 0; i <= maxNumSubLayersMinus1; i++ )");

        for(std::size_t i = 0; i < hrd.fixed_pic_rate_general_flag.size(); i++ )
        {
            AppendItem(pitem, wxString::Format("fixed_pic_rate_general_flag[%lld] = %lld", (long long)i, (long long)hrd.fixed_pic_rate_general_flag[i]));

            if(!hrd.fixed_pic_rate_general_flag[i])
            {
                pitemSecond = AppendItem(pitem, wxString::Format("if( !fixed_pic_rate_general_flag [%lld] )", (long long)i));
                AppendItem(pitemSecond, wxString::Format("fixed_pic_rate_within_cvs_flag[%lld] = %lld", (long long)i, (long long)hrd.fixed_pic_rate_within_cvs_flag[i]));
            }

            if(hrd.fixed_pic_rate_within_cvs_flag[i])
            {
                pitemSecond = AppendItem(pitem, wxString::Format("if( fixed_pic_rate_within_cvs_flag[%lld] )", (long long)i));
                AppendItem(pitemSecond, wxString::Format("elemental_duration_in_tc_minus1[%lld] = %lld", (long long)i, (long long)hrd.elemental_duration_in_tc_minus1[i]));
            }
            else
            {
                pitemSecond = AppendItem(pitem, wxString::Format("if( !fixed_pic_rate_within_cvs_flag[%lld] )", (long long)i));
                AppendItem(pitemSecond, wxString::Format("low_delay_hrd_flag[%lld] = %lld", (long long)i, (long long)hrd.low_delay_hrd_flag[i]));
            }

            if(!hrd.low_delay_hrd_flag[i])
            {
                pitemSecond = AppendItem(pitem, wxString::Format("if( !low_delay_hrd_flag[%lld] )", (long long)i));
                AppendItem(pitemSecond, wxString::Format("cpb_cnt_minus1[%lld] = %lld", (long long)i, (long long)hrd.cpb_cnt_minus1[i]));
            }

            if(hrd.nal_hrd_parameters_present_flag)
            {
                pitemSecond = AppendItem(pitem, "if( nal_hrd_parameters_present_flag )");

                pitemThird = AppendItem(pitemSecond, wxString::Format("sub_layer_hrd_parameters(%lld)", (long long)i));

                createSubLayerHrdParameters(hrd.nal_sub_layer_hrd_parameters[i], hrd.sub_pic_hrd_params_present_flag, pitemThird);
            }
            if(hrd.vcl_hrd_parameters_present_flag)
            {
                pitemSecond = AppendItem(pitem, "if( vcl_hrd_parameters_present_flag )");

                pitemThird = AppendItem(pitemSecond, wxString::Format("sub_layer_hrd_parameters(%lld)", (long long)i));

                createSubLayerHrdParameters(hrd.vcl_sub_layer_hrd_parameters[i], hrd.sub_pic_hrd_params_present_flag, pitemThird);
            }
        }
    }
}

void SyntaxViewer::createSubLayerHrdParameters(const HEVC::SubLayerHrdParameters &slhrd, uint8_t sub_pic_hrd_params_present_flag, wxTreeItemId pSlhrdItem)
{
    wxTreeItemId pitem = AppendItem(pSlhrdItem, "for( i = 0; i <= CpbCnt; i++ )");
    wxTreeItemId pitemSecond;

    for(std::size_t i=0; i<slhrd.bit_rate_value_minus1.size(); i++)
    {
        AppendItem(pitem, wxString::Format("bit_rate_value_minus1[%lld] = %lld", (long long)i, (long long)slhrd.bit_rate_value_minus1[i]));
        AppendItem(pitem, wxString::Format("cpb_size_value_minus1[%lld] = %lld", (long long)i, (long long)slhrd.cpb_size_value_minus1[i]));

        if(sub_pic_hrd_params_present_flag)
        {
            pitemSecond = AppendItem(pitem, "if( sub_pic_hrd_params_present_flag )");

            AppendItem(pitemSecond, wxString::Format("cpb_size_du_value_minus1[%lld] = %lld", (long long)i, (long long)slhrd.cpb_size_du_value_minus1[i]));
            AppendItem(pitemSecond, wxString::Format("bit_rate_du_value_minus1[%lld] = %lld", (long long)i, (long long)slhrd.bit_rate_du_value_minus1[i]));
        }

        AppendItem(pitem, wxString::Format("cbr_flag[%lld] = %lld", (long long)i, (long long)slhrd.cbr_flag[i]));
    }
}


void SyntaxViewer::createShortTermRefPicSet(std::size_t stRpsIdx, const HEVC::ShortTermRefPicSet &rpset, std::size_t num_short_term_ref_pic_sets, const std::vector<HEVC::ShortTermRefPicSet> &refPicSets, wxTreeItemId pStrpsItem)
{
    wxTreeItemId pitem;
    wxTreeItemId pitemSecond;
    wxTreeItemId pitemLoop;

    if(stRpsIdx)
    {
        pitem = AppendItem(pStrpsItem, "if( stRpsIdx )");
        AppendItem(pitem, wxString::Format("inter_ref_pic_set_prediction_flag = %lld", (long long)rpset.inter_ref_pic_set_prediction_flag));
    }

    if(rpset.inter_ref_pic_set_prediction_flag)
    {
        pitem = AppendItem(pStrpsItem, "if( inter_ref_pic_set_prediction_flag )");

        if(stRpsIdx == num_short_term_ref_pic_sets)
        {
            pitemSecond = AppendItem(pitem, "if( stRpsIdx == num_short_term_ref_pic_sets )");
            AppendItem(pitemSecond, wxString::Format("delta_idx_minus1 = %lld", (long long)rpset.delta_idx_minus1));
        }

        AppendItem(pitem, wxString::Format("delta_rps_sign = %lld", (long long)rpset.delta_rps_sign));
        AppendItem(pitem, wxString::Format("abs_delta_rps_minus1 = %lld", (long long)rpset.abs_delta_rps_minus1));

        std::size_t RefRpsIdx = stRpsIdx - (rpset.delta_idx_minus1 + 1);

        std::size_t NumDeltaPocs = 0;

        if(refPicSets[RefRpsIdx].inter_ref_pic_set_prediction_flag)
        {
            for(std::size_t i=0; i<refPicSets[RefRpsIdx].used_by_curr_pic_flag.size(); i++)
                if(refPicSets[RefRpsIdx].used_by_curr_pic_flag[i] || refPicSets[RefRpsIdx].use_delta_flag[i])
                    NumDeltaPocs++;
        }
        else
            NumDeltaPocs = refPicSets[RefRpsIdx].num_negative_pics + refPicSets[RefRpsIdx].num_positive_pics;


        pitemLoop = AppendItem(pitem, "for( j = 0; j <= NumDeltaPocs[ RefRpsIdx ]; j++ )");

        for(std::size_t i=0; i<=NumDeltaPocs; i++ )
        {
            AppendItem(pitemLoop, wxString::Format("used_by_curr_pic_flag[%lld] = %lld", (long long)i, (long long)rpset.used_by_curr_pic_flag[i]));

            if(!rpset.used_by_curr_pic_flag[i])
            {
                pitem = AppendItem(pitemLoop, "if( !used_by_curr_pic_flag[j] )");
                AppendItem(pitem, wxString::Format("use_delta_flag[%lld] = %lld", (long long)i, (long long)rpset.use_delta_flag[i]));

            }
        }
    }
    else
    {
        pitem = AppendItem(pStrpsItem, "if( !inter_ref_pic_set_prediction_flag )");

        AppendItem(pitem, wxString::Format("num_negative_pics = %lld", (long long)rpset.num_negative_pics));
        AppendItem(pitem, wxString::Format("num_positive_pics = %lld", (long long)rpset.num_positive_pics));

        pitemLoop = AppendItem(pitem, "for( i = 0; i < num_negative_pics; i++ )");

        for(std::size_t i=0; i<rpset.num_negative_pics; i++)
        {
            AppendItem(pitemLoop, wxString::Format("delta_poc_s0_minus1[%lld] = %lld", (long long)i, (long long)rpset.delta_poc_s0_minus1[i]));
            AppendItem(pitemLoop, wxString::Format("used_by_curr_pic_s0_flag[%lld] = %lld", (long long)i, (long long)rpset.used_by_curr_pic_s0_flag[i]));
        }

        pitemLoop = AppendItem(pitem, "for( i = 0; i < num_positive_pics; i++ )");

        for(std::size_t i=0; i<rpset.num_positive_pics; i++)
        {
            AppendItem(pitemLoop, wxString::Format("delta_poc_s1_minus1[%lld] = %lld", (long long)i, (long long)rpset.delta_poc_s1_minus1[i]));
            AppendItem(pitemLoop, wxString::Format("used_by_curr_pic_s1_flag[%lld] = %lld", (long long)i, (long long)rpset.used_by_curr_pic_s1_flag[i]));
        }
    }
}


void SyntaxViewer::createScalingListData(const HEVC::ScalingListData &scdata, wxTreeItemId pScItem)
{
    wxTreeItemId pitem;
    wxTreeItemId pitemSecond;

    pitem = AppendItem(pScItem, "for( sizeId = 0; sizeId < 4; sizeId++ )");
    pitemSecond = AppendItem(pitem, "for( matrixId = 0; matrixId < ( ( sizeId == 3 ) ? 2 : 6 ); matrixId++ )");


    for(std::size_t sizeId = 0; sizeId < 4; sizeId++)
    {
        for(std::size_t matrixId = 0; matrixId<((sizeId == 3)?2:6); matrixId++)
        {
            AppendItem(pitemSecond, wxString::Format("scaling_list_pred_mode_flag[%lld][%lld] = %lld", (long long)sizeId, (long long)matrixId, (long long)scdata.scaling_list_pred_mode_flag[sizeId][matrixId]));

            if(!scdata.scaling_list_pred_mode_flag[sizeId][matrixId])
            {
                pitem = AppendItem(pitemSecond, "if( !scaling_list_pred_mode_flag[ sizeId ][ matrixId ] )");
                AppendItem(pitem, wxString::Format("scaling_list_pred_matrix_id_delta[%lld][%lld] = %lld", (long long)sizeId, (long long)matrixId, (long long)scdata.scaling_list_pred_matrix_id_delta[sizeId][matrixId]));
            }
            else
            {
                std::size_t coefNum = std::min((std::size_t)64, (std::size_t)(1 << (4 + (sizeId << 1))));
                if(sizeId > 1)
                {
                    pitem = AppendItem(pitemSecond, "if( sizeId > 1 )");
                    AppendItem(pitem, wxString::Format("scaling_list_dc_coef_minus8[%lld][%lld] = %lld", (long long)sizeId, (long long)matrixId, (long long)scdata.scaling_list_dc_coef_minus8[sizeId-2][matrixId]));
                }

                pitem = AppendItem(pitemSecond, "for( i = 0; i < coefNum; i++ )");

                for(std::size_t i = 0; i < coefNum; i++)
                {
                    AppendItem(pitem, wxString::Format("scaling_list_delta_coef[%lld][%lld][%lld] = %lld", (long long)sizeId, (long long)matrixId, (long long)i, (long long)scdata.scaling_list_delta_coef[sizeId][matrixId][i]));
                }
            }
        }
    }
}


void SyntaxViewer::createRefPicListModification(const HEVC::RefPicListModification &rplModif, wxTreeItemId pRplmItem)
{
    wxTreeItemId pitem;

    AppendItem(pRplmItem, wxString::Format("ref_pic_list_modification_flag_l0 = %lld", (long long)rplModif.ref_pic_list_modification_flag_l0));

    if(rplModif.ref_pic_list_modification_flag_l0)
    {
        pitem = AppendItem(pRplmItem, "for( i = 0; i <= num_ref_idx_l0_active_minus1; i++ )");

        for(std::size_t i=0; i<rplModif.list_entry_l0.size(); i++)
            AppendItem(pitem, wxString::Format("list_entry_l0[%lld] = %lld", (long long)i, (long long)rplModif.list_entry_l0[i]));
    }

    AppendItem(pRplmItem, wxString::Format("ref_pic_list_modification_flag_l1 = %lld", (long long)rplModif.ref_pic_list_modification_flag_l1));

    if(rplModif.ref_pic_list_modification_flag_l1)
    {
        pitem = AppendItem(pRplmItem, "for( i = 0; i <= num_ref_idx_l1_active_minus1; i++ )");

        for(std::size_t i=0; i<rplModif.list_entry_l1.size(); i++)
            AppendItem(pitem, wxString::Format("list_entry_l1[%lld] = %lld", (long long)i, (long long)rplModif.list_entry_l1[i]));
    }
}


void SyntaxViewer::createPredWeightTable(const HEVC::PredWeightTable &pwt, std::shared_ptr<HEVC::Slice> pSlice, wxTreeItemId ppwtItem)
{
    std::shared_ptr<HEVC::PPS> ppps = m_ppsMap[pSlice -> slice_pic_parameter_set_id];
    if(!ppps)
        return ;

    std::shared_ptr<HEVC::SPS> psps = m_spsMap[ppps -> pps_seq_parameter_set_id];

    if(!psps)
        return ;

    wxTreeItemId pitem;
    wxTreeItemId pitemLoop;
    wxTreeItemId pitemBSlice;

    AppendItem(ppwtItem, wxString::Format("luma_log2_weight_denom = %lld", (long long)pwt.luma_log2_weight_denom));

    if(psps -> chroma_format_idc != 0)
    {
        pitem = AppendItem(ppwtItem, "if (psps -> chroma_format_idc)");

        AppendItem(pitem, wxString::Format("delta_chroma_log2_weight_denom = %lld", (long long)pwt.delta_chroma_log2_weight_denom));
    }

    pitemLoop = AppendItem(ppwtItem, "for(i=0; i<=num_ref_idx_l0_active_minus1; i++)");

    for(std::size_t i=0; i<=pSlice -> num_ref_idx_l0_active_minus1; i++)
        AppendItem(pitemLoop, wxString::Format("luma_weight_l0_flag[%lld] = %lld", (long long)i, (long long)pwt.luma_weight_l0_flag[i]));

    if(psps -> chroma_format_idc != 0)
    {
        pitem = AppendItem(ppwtItem, "if (psps -> chroma_format_idc)");

        pitemLoop = AppendItem(pitem, "for(i=0; i<=num_ref_idx_l0_active_minus1; i++)");

        for(std::size_t i=0; i<=pSlice -> num_ref_idx_l0_active_minus1; i++)
            AppendItem(pitemLoop, wxString::Format("chroma_weight_l0_flag[%lld] = %lld", (long long)i, (long long)pwt.chroma_weight_l0_flag[i]));
    }


    for(std::size_t i=0; i<=pSlice -> num_ref_idx_l0_active_minus1; i++)
    {
        pitemLoop = AppendItem(ppwtItem, "for(i=0; i<=num_ref_idx_l0_active_minus1; i++)");

        if(pwt.luma_weight_l0_flag[i])
        {
            pitem = AppendItem(pitemLoop, wxString::Format("if (luma_weight_l0_flag[%lld])", (long long)i));

            AppendItem(pitem, wxString::Format("delta_luma_weight_l0[%lld] = %lld", (long long)i, (long long)pwt.delta_luma_weight_l0[i]));
            AppendItem(pitem, wxString::Format("luma_offset_l0[%lld] = %lld", (long long)i, (long long)pwt.luma_offset_l0[i]));
        }
        if(pwt.chroma_weight_l0_flag[i])
        {
            pitem = AppendItem(pitemLoop, wxString::Format("if (chroma_weight_l0_flag[%lld])", (long long)i));

            AppendItem(pitem, wxString::Format("delta_chroma_weight_l0[%lld] = { %lld, %lld } ", (long long)i, (long long)pwt.delta_chroma_weight_l0[i][0], (long long)pwt.delta_chroma_weight_l0[i][1]));
            AppendItem(pitem, wxString::Format("delta_chroma_offset_l0[%lld] = { %lld, %lld } ", (long long)i, (long long)pwt.delta_chroma_offset_l0[i][0], (long long)pwt.delta_chroma_offset_l0[i][1]));
        }
    }

    if(pSlice -> slice_type == SLICE_B)
    {
        pitemBSlice = AppendItem(ppwtItem, "slice_type == B");

        pitemLoop = AppendItem(pitemBSlice, "for(i=0; i<=num_ref_idx_l1_active_minus1; i++)");

        for(std::size_t i=0; i<=pSlice -> num_ref_idx_l1_active_minus1; i++)
            AppendItem(pitemLoop, wxString::Format("luma_weight_l1_flag[%lld] = %lld", (long long)i, (long long)pwt.luma_weight_l1_flag[i]));

        if(psps -> chroma_format_idc != 0)
        {
            pitem = AppendItem(pitemBSlice, "if (psps -> chroma_format_idc)");

            pitemLoop = AppendItem(pitem, "for(i=0; i<=num_ref_idx_l1_active_minus1; i++)");

            for(std::size_t i=0; i<=pSlice -> num_ref_idx_l1_active_minus1; i++)
                AppendItem(pitemLoop, wxString::Format("chroma_weight_l1_flag[%lld] = %lld", (long long)i, (long long)pwt.chroma_weight_l1_flag[i]));
        }


        for(std::size_t i=0; i<=pSlice -> num_ref_idx_l1_active_minus1; i++)
        {
            pitemLoop = AppendItem(pitemBSlice, "for(i=0; i<=num_ref_idx_l1_active_minus1; i++)");

            if(pwt.luma_weight_l1_flag[i])
            {
                pitem = AppendItem(pitemLoop, wxString::Format("if (luma_weight_l1_flag[%lld])", (long long)i));

                AppendItem(pitem, wxString::Format("delta_luma_weight_l1[%lld] = %lld", (long long)i, (long long)pwt.delta_luma_weight_l1[i]));
                AppendItem(pitem, wxString::Format("luma_offset_l1[%lld] = %lld", (long long)i, (long long)pwt.luma_offset_l1[i]));
            }
            if(pwt.chroma_weight_l1_flag[i])
            {
                pitem = AppendItem(pitemLoop, wxString::Format("if (chroma_weight_l1_flag[%lld])", (long long)i));

                AppendItem(pitem, wxString::Format("delta_chroma_weight_l1[%lld] = { %lld, %lld } ", (long long)i, (long long)pwt.delta_chroma_weight_l1[i][0], (long long)pwt.delta_chroma_weight_l1[i][1]));
                AppendItem(pitem, wxString::Format("delta_chroma_offset_l1[%lld] = { %lld, %lld } ", (long long)i, (long long)pwt.delta_chroma_offset_l1[i][0], (long long)pwt.delta_chroma_offset_l1[i][1]));
            }
        }
    }

}

void SyntaxViewer::createDecodedPictureHash(std::shared_ptr<HEVC::DecodedPictureHash> pDecPictHash, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("hash_type = %lld", (long long)pDecPictHash->hash_type));

    wxTreeItemId ploop = AppendItem(pItem, "for( cIdx = 0; cIdx < ( chroma_format_idc = = 0 ? 1 : 3 ); cIdx++ )");

    wxTreeItemId pif = AppendItem(ploop, wxString::Format("if(hash_type = %lld) ", (long long)pDecPictHash->hash_type));


    if(pDecPictHash->hash_type == 0)
    {
        for(std::size_t i=0; i<pDecPictHash->picture_md5.size(); i++)
        {
            wxString str;
            for(std::size_t j=0; j<16; j++)
                str += wxString::Format("%02X", (unsigned int)pDecPictHash->picture_md5[i][j]);
            AppendItem(pif, wxString::Format("picture_md5[%lld] = %s", (long long)i, str));
        }
    }
    if(pDecPictHash->hash_type == 1)
    {
        for(std::size_t i=0; i<pDecPictHash->picture_crc.size(); i++)
        {
            AppendItem(pif, wxString::Format("picture_crc[%lld] = %lld", (long long)i, (long long)pDecPictHash -> picture_crc[i]));
        }
    }
    else
    {
        for(std::size_t i=0; i<pDecPictHash->picture_crc.size(); i++)
        {
            AppendItem(pif, wxString::Format("picture_checksum[%lld] = %lld", (long long)i, (long long)pDecPictHash -> picture_checksum[i]));
        }
    }
}


void SyntaxViewer::createMasteringDisplayInfo(std::shared_ptr<HEVC::MasteringDisplayInfo> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemLoop = AppendItem(pItem, "for( i = 0; i < 3 ; i++ )");


    for (uint32_t i = 0; i < 3; i++)
    {
        AppendItem(pitemLoop, wxString::Format("display_primary_x[%lld] = %lld", (long long)i, (long long)pSei -> display_primary_x[i]));
        AppendItem(pitemLoop, wxString::Format("display_primary_y[%lld] = %lld", (long long)i, (long long)pSei -> display_primary_y[i]));
    }

    AppendItem(pItem, wxString::Format("white_point_x = %lld", (long long)pSei -> white_point_x));
    AppendItem(pItem, wxString::Format("white_point_y = %lld", (long long)pSei -> white_point_y));

    AppendItem(pItem, wxString::Format("max_display_mastering_luminance = %lld", (long long)pSei -> max_display_mastering_luminance));
    AppendItem(pItem, wxString::Format("min_display_mastering_luminance = %lld", (long long)pSei -> min_display_mastering_luminance));
}


void SyntaxViewer::createActiveParameterSets(std::shared_ptr<HEVC::ActiveParameterSets> pSeiPayload, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("active_video_parameter_set_id = %lld", (long long)pSeiPayload -> active_video_parameter_set_id));
    AppendItem(pItem, wxString::Format("self_contained_cvs_flag = %lld", (long long)pSeiPayload -> self_contained_cvs_flag));
    AppendItem(pItem, wxString::Format("no_parameter_set_update_flag = %lld", (long long)pSeiPayload -> no_parameter_set_update_flag));
    AppendItem(pItem, wxString::Format("num_sps_ids_minus1 = %lld", (long long)pSeiPayload -> num_sps_ids_minus1));

    wxString str = "active_seq_parameter_set_id = {";
    for (uint32_t i = 0; i < pSeiPayload -> num_sps_ids_minus1; i++)
    {
        str += wxString::Format("%lld,  ", (long long)pSeiPayload -> active_seq_parameter_set_id[i]);
    }

    str += wxString::Format("%lld\n}", (long long)pSeiPayload -> active_seq_parameter_set_id[pSeiPayload -> num_sps_ids_minus1]);
    AppendItem(pItem, str);
}


void SyntaxViewer::createUserDataUnregistered(std::shared_ptr<HEVC::UserDataUnregistered> pSei, wxTreeItemId pItem)
{
    wxString str =
        wxString::Format("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            (unsigned int)pSei -> uuid_iso_iec_11578[0], (unsigned int)pSei -> uuid_iso_iec_11578[1],
            (unsigned int)pSei -> uuid_iso_iec_11578[2], (unsigned int)pSei -> uuid_iso_iec_11578[3],
            (unsigned int)pSei -> uuid_iso_iec_11578[4], (unsigned int)pSei -> uuid_iso_iec_11578[5],
            (unsigned int)pSei -> uuid_iso_iec_11578[6], (unsigned int)pSei -> uuid_iso_iec_11578[7],
            (unsigned int)pSei -> uuid_iso_iec_11578[8], (unsigned int)pSei -> uuid_iso_iec_11578[9],
            (unsigned int)pSei -> uuid_iso_iec_11578[10], (unsigned int)pSei -> uuid_iso_iec_11578[11],
            (unsigned int)pSei -> uuid_iso_iec_11578[12], (unsigned int)pSei -> uuid_iso_iec_11578[13],
            (unsigned int)pSei -> uuid_iso_iec_11578[14], (unsigned int)pSei -> uuid_iso_iec_11578[15]);

    AppendItem(pItem, wxString::Format("uuid_iso_iec_11578 = %s", str));


    if(pSei -> user_data_payload_byte.empty())
    {
        str = "user_data_payload_byte = { }";
    }
    else
    {
        str = "user_data_payload_byte = {";
        for (std::size_t i = 0; i < pSei -> user_data_payload_byte.size() - 1; i++)
        {
            str += wxString::Format("%02x", (unsigned int)pSei -> user_data_payload_byte[i]);
        }
        str += wxString::Format("%02x\n}", (unsigned int)pSei -> user_data_payload_byte[pSei -> user_data_payload_byte.size() - 1]);
    }

    AppendItem(pItem, str);
}


void SyntaxViewer::createBufferingPeriod(std::shared_ptr<HEVC::BufferingPeriod> pSei, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("bp_seq_parameter_set_id = %lld", (long long)pSei->bp_seq_parameter_set_id));

    std::shared_ptr<HEVC::SPS> psps = m_spsMap[pSei -> bp_seq_parameter_set_id];
    if(!psps)
        return;

    wxTreeItemId pitemIrap;
    wxTreeItemId pitemBpPresentFlag;
    wxTreeItemId ploop;
    wxTreeItemId pitemAlt;

    if(!psps->vui_parameters.hrd_parameters.sub_pic_hrd_params_present_flag)
    {
        pitemIrap = AppendItem(pItem, "if( !sub_pic_hrd_params_present_flag )");
        AppendItem(pitemIrap, wxString::Format("irap_cpb_params_present_flag = %lld", (long long)pSei->irap_cpb_params_present_flag));
    }

    if(pSei -> irap_cpb_params_present_flag)
    {
        pitemIrap = AppendItem(pItem, "if( irap_cpb_params_present_flag )");
        AppendItem(pitemIrap, wxString::Format("cpb_delay_offset = %lld", (long long)pSei->cpb_delay_offset));
        AppendItem(pitemIrap, wxString::Format("dpb_delay_offset = %lld", (long long)pSei->dpb_delay_offset));
    }

    AppendItem(pItem, wxString::Format("concatenation_flag = %lld", (long long)pSei->concatenation_flag));
    AppendItem(pItem, wxString::Format("au_cpb_removal_delay_delta_minus1 = %lld", (long long)pSei->au_cpb_removal_delay_delta_minus1));

    bool NalHrdBpPresentFlag = psps->vui_parameters.hrd_parameters.nal_hrd_parameters_present_flag;
    if(NalHrdBpPresentFlag)
    {
        pitemBpPresentFlag = AppendItem(pItem, "if( NalHrdBpPresentFlag )");

        ploop = AppendItem(pitemBpPresentFlag, "for( i = 0; i <= CpbCnt; i++ )");

        for(std::size_t i=0; i<pSei->nal_initial_cpb_removal_delay.size(); i++)
        {
            AppendItem(ploop, wxString::Format("nal_initial_cpb_removal_delay[%lld] = %lld", (long long)i, (long long)pSei -> nal_initial_cpb_removal_delay[i]));
            AppendItem(ploop, wxString::Format("nal_initial_cpb_removal_offset[%lld] = %lld", (long long)i, (long long)pSei -> nal_initial_cpb_removal_offset[i]));

            if(psps->vui_parameters.hrd_parameters.sub_pic_hrd_params_present_flag || pSei->irap_cpb_params_present_flag)
            {
                pitemAlt = AppendItem(ploop, "if( sub_pic_hrd_params_present_flag | | irap_cpb_params_present_flag )");

                AppendItem(pitemAlt, wxString::Format("nal_initial_alt_cpb_removal_delay[%lld] = %lld", (long long)i, (long long)pSei -> nal_initial_alt_cpb_removal_delay[i]));
                AppendItem(pitemAlt, wxString::Format("nal_initial_alt_cpb_removal_offset[%lld] = %lld", (long long)i, (long long)pSei -> nal_initial_alt_cpb_removal_offset[i]));
            }

        }
    }

    bool VclHrdBpPresentFlag = psps->vui_parameters.hrd_parameters.vcl_hrd_parameters_present_flag;
    if(VclHrdBpPresentFlag)
    {
        pitemBpPresentFlag = AppendItem(pItem, "if( VclHrdBpPresentFlag )");

        ploop = AppendItem(pitemBpPresentFlag, "for( i = 0; i <= CpbCnt; i++ )");

        for(std::size_t i=0; i<pSei->vcl_initial_cpb_removal_delay.size(); i++)
        {
            AppendItem(ploop, wxString::Format("vcl_initial_cpb_removal_delay[%lld] = %lld", (long long)i, (long long)pSei -> vcl_initial_cpb_removal_delay[i]));
            AppendItem(ploop, wxString::Format("vcl_initial_cpb_removal_offset[%lld] = %lld", (long long)i, (long long)pSei -> vcl_initial_cpb_removal_offset[i]));

            if(psps->vui_parameters.hrd_parameters.sub_pic_hrd_params_present_flag || pSei->irap_cpb_params_present_flag)
            {
                pitemAlt = AppendItem(ploop, "if( sub_pic_hrd_params_present_flag | | irap_cpb_params_present_flag )");

                AppendItem(pitemAlt, wxString::Format("vcl_initial_alt_cpb_removal_delay[%lld] = %lld", (long long)i, (long long)pSei -> vcl_initial_alt_cpb_removal_delay[i]));
                AppendItem(pitemAlt, wxString::Format("vcl_initial_alt_cpb_removal_offset[%lld] = %lld", (long long)i, (long long)pSei -> vcl_initial_alt_cpb_removal_offset[i]));
            }
        }
    }
}

void SyntaxViewer::createPicTiming(std::shared_ptr<HEVC::PicTiming> pSei, wxTreeItemId pItem)
{
    std::shared_ptr<HEVC::SPS> psps;

    if(m_spsMap.size())
        psps = m_spsMap.begin() -> second;

    if(!psps)
        return;

    wxTreeItemId pitemField;
    wxTreeItemId pitemDpbDelays;
    wxTreeItemId pitemDuDelay;
    wxTreeItemId pitemIf;
    wxTreeItemId pitemDuCommCpb;
    wxTreeItemId ploop;


    if(psps->vui_parameters.frame_field_info_present_flag)
    {
        pitemField = AppendItem(pItem, "if( frame_field_info_present_flag )");

        AppendItem(pitemField, wxString::Format("pic_struct = %lld", (long long)pSei->pic_struct));
        AppendItem(pitemField, wxString::Format("source_scan_type = %lld", (long long)pSei->source_scan_type));
        AppendItem(pitemField, wxString::Format("duplicate_flag = %lld", (long long)pSei->duplicate_flag));
    }

    bool CpbDpbDelaysPresentFlag = psps->vui_parameters.hrd_parameters.nal_hrd_parameters_present_flag ||
                                    psps->vui_parameters.hrd_parameters.vcl_hrd_parameters_present_flag;

    if(CpbDpbDelaysPresentFlag)
    {
        pitemDpbDelays = AppendItem(pItem, "if( CpbDpbDelaysPresentFlag )");

        AppendItem(pitemDpbDelays, wxString::Format("au_cpb_removal_delay_minus1 = %lld", (long long)pSei->au_cpb_removal_delay_minus1));
        AppendItem(pitemDpbDelays, wxString::Format("pic_dpb_output_delay = %lld", (long long)pSei->pic_dpb_output_delay));

        if(psps->vui_parameters.hrd_parameters.sub_pic_hrd_params_present_flag)
        {
            pitemDuDelay = AppendItem(pitemDpbDelays, "if( CpbDpbDelaysPresentFlag )");

            AppendItem(pitemDuDelay, wxString::Format("pic_dpb_output_du_delay = %lld", (long long)pSei->pic_dpb_output_du_delay));
        }

        if(psps->vui_parameters.hrd_parameters.sub_pic_hrd_params_present_flag &&
            psps->vui_parameters.hrd_parameters.sub_pic_cpb_params_in_pic_timing_sei_flag)
        {
            pitemIf = AppendItem(pitemDpbDelays, "if( CpbDpbDelaysPresentFlag )");

            AppendItem(pitemIf, wxString::Format("num_decoding_units_minus1 = %lld", (long long)pSei->num_decoding_units_minus1));
            AppendItem(pitemIf, wxString::Format("du_common_cpb_removal_delay_flag = %lld", (long long)pSei->du_common_cpb_removal_delay_flag));

            if(pSei -> du_common_cpb_removal_delay_flag)
            {
                pitemDuCommCpb = AppendItem(pitemIf, "if( CpbDpbDelaysPresentFlag )");

                AppendItem(pitemDuCommCpb, wxString::Format("du_common_cpb_removal_delay_increment_minus1 = %lld", (long long)pSei->num_decoding_units_minus1));
            }

            ploop = AppendItem(pitemIf, "for( i = 0; i <= num_decoding_units_minus1; i++ )");

            for(std::size_t i=0; i<=pSei -> num_decoding_units_minus1; i++)
            {
                AppendItem(ploop, wxString::Format("num_nalus_in_du_minus1[%lld] = %lld", (long long)i, (long long)pSei -> num_nalus_in_du_minus1[i]));

                if(!pSei -> du_common_cpb_removal_delay_flag && i<pSei -> num_decoding_units_minus1)
                {
                    pitemDuCommCpb = AppendItem(ploop, "if( !du_common_cpb_removal_delay_flag && i < num_decoding_units_minus1 )");

                    AppendItem(pitemDuCommCpb, wxString::Format("du_cpb_removal_delay_increment_minus1[%lld] = %lld", (long long)i, (long long)pSei -> du_cpb_removal_delay_increment_minus1[i]));
                }
            }
        }
    }
}


void SyntaxViewer::createRecoveryPoint(std::shared_ptr<HEVC::RecoveryPoint> pSei, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("recovery_poc_cnt = %lld", (long long)pSei->recovery_poc_cnt));
    AppendItem(pItem, wxString::Format("exact_match_flag = %lld", (long long)pSei->exact_match_flag));
    AppendItem(pItem, wxString::Format("broken_link_flag = %lld", (long long)pSei->broken_link_flag));
}


void SyntaxViewer::createContentLightLevelInfo(std::shared_ptr<HEVC::ContentLightLevelInfo> pSeiPayload, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("max_content_light_level = %lld", (long long)pSeiPayload->max_content_light_level));
    AppendItem(pItem, wxString::Format("max_pic_average_light_level = %lld", (long long)pSeiPayload->max_pic_average_light_level));
}

void SyntaxViewer::createAlternativeTransferCharacteristics(std::shared_ptr<HEVC::AlternativeTransferCharacteristics> pSeiPayload, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("alternative_transfer_characteristics = %lld", (long long)pSeiPayload->alternative_transfer_characteristics));
}


void SyntaxViewer::createFramePacking(std::shared_ptr<HEVC::FramePacking> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemIf;
    wxTreeItemId pitemGridPos;

    AppendItem(pItem, wxString::Format("frame_packing_arrangement_id = %lld", (long long)pSei->frame_packing_arrangement_id));
    AppendItem(pItem, wxString::Format("frame_packing_arrangement_cancel_flag = %lld", (long long)pSei->frame_packing_arrangement_cancel_flag));

    if(!pSei -> frame_packing_arrangement_cancel_flag)
    {
        pitemIf = AppendItem(pItem, "if( !frame_packing_arrangement_cancel_flag )");

        AppendItem(pitemIf, wxString::Format("frame_packing_arrangement_type = %lld", (long long)pSei->frame_packing_arrangement_type));
        AppendItem(pitemIf, wxString::Format("quincunx_sampling_flag = %lld", (long long)pSei->quincunx_sampling_flag));
        AppendItem(pitemIf, wxString::Format("content_interpretation_type = %lld", (long long)pSei->content_interpretation_type));
        AppendItem(pitemIf, wxString::Format("spatial_flipping_flag = %lld", (long long)pSei->spatial_flipping_flag));
        AppendItem(pitemIf, wxString::Format("frame0_flipped_flag = %lld", (long long)pSei->frame0_flipped_flag));
        AppendItem(pitemIf, wxString::Format("field_views_flag = %lld", (long long)pSei->field_views_flag));
        AppendItem(pitemIf, wxString::Format("current_frame_is_frame0_flag = %lld", (long long)pSei->current_frame_is_frame0_flag));
        AppendItem(pitemIf, wxString::Format("frame0_self_contained_flag = %lld", (long long)pSei->frame0_self_contained_flag));
        AppendItem(pitemIf, wxString::Format("frame1_self_contained_flag = %lld", (long long)pSei->frame1_self_contained_flag));

        if(!pSei -> quincunx_sampling_flag && pSei -> frame_packing_arrangement_type != 5)
        {
            pitemGridPos = AppendItem(pitemIf, "if( !quincunx_sampling_flag && frame_packing_arrangement_type != 5 )");

            AppendItem(pitemGridPos, wxString::Format("frame0_grid_position_x = %lld", (long long)pSei->frame0_grid_position_x));
            AppendItem(pitemGridPos, wxString::Format("frame0_grid_position_y = %lld", (long long)pSei->frame0_grid_position_y));
            AppendItem(pitemGridPos, wxString::Format("frame1_grid_position_x = %lld", (long long)pSei->frame1_grid_position_x));
            AppendItem(pitemGridPos, wxString::Format("frame1_grid_position_y = %lld", (long long)pSei->frame1_grid_position_y));
        }

        AppendItem(pitemIf, wxString::Format("frame_packing_arrangement_reserved_byte = %lld", (long long)pSei->frame_packing_arrangement_reserved_byte));
        AppendItem(pitemIf, wxString::Format("frame_packing_arrangement_persistence_flag = %lld", (long long)pSei->frame_packing_arrangement_persistence_flag));

    }

    AppendItem(pItem, wxString::Format("upsampled_aspect_ratio_flag = %lld", (long long)pSei->upsampled_aspect_ratio_flag));
}


void SyntaxViewer::createDisplayOrientation(std::shared_ptr<HEVC::DisplayOrientation> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemIf;

    AppendItem(pItem, wxString::Format("display_orientation_cancel_flag = %lld", (long long)pSei->display_orientation_cancel_flag));
    if(!pSei -> display_orientation_cancel_flag)
    {
        pitemIf = AppendItem(pItem, "if( !display_orientation_cancel_flag )");

        AppendItem(pitemIf, wxString::Format("hor_flip = %lld", (long long)pSei->hor_flip));
        AppendItem(pitemIf, wxString::Format("ver_flip = %lld", (long long)pSei->ver_flip));
        AppendItem(pitemIf, wxString::Format("anticlockwise_rotation = %lld", (long long)pSei->anticlockwise_rotation));
        AppendItem(pitemIf, wxString::Format("display_orientation_persistence_flag = %lld", (long long)pSei->display_orientation_persistence_flag));
    }
}


void SyntaxViewer::createToneMapping(std::shared_ptr<HEVC::ToneMapping> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemIf;
    wxTreeItemId pitemSecond;
    wxTreeItemId ploop;
    wxTreeItemId pitemThird;

    AppendItem(pItem, wxString::Format("tone_map_id = %lld", (long long)pSei->tone_map_id));
    AppendItem(pItem, wxString::Format("tone_map_cancel_flag = %lld", (long long)pSei->tone_map_cancel_flag));

    if(!pSei -> tone_map_cancel_flag)
    {
        pitemIf = AppendItem(pItem, "if( !tone_map_cancel_flag )");

        AppendItem(pitemIf, wxString::Format("tone_map_persistence_flag = %lld", (long long)pSei->tone_map_persistence_flag));
        AppendItem(pitemIf, wxString::Format("coded_data_bit_depth = %lld", (long long)pSei->coded_data_bit_depth));
        AppendItem(pitemIf, wxString::Format("target_bit_depth = %lld", (long long)pSei->target_bit_depth));
        AppendItem(pitemIf, wxString::Format("tone_map_model_id = %lld", (long long)pSei->tone_map_model_id));

        if(pSei -> tone_map_model_id == 0)
        {
            pitemSecond = AppendItem(pitemIf, "if( tone_map_model_id = = 0 )");

            AppendItem(pitemSecond, wxString::Format("min_value = %lld", (long long)pSei->min_value));
            AppendItem(pitemSecond, wxString::Format("max_value = %lld", (long long)pSei->max_value));
        }
        else if(pSei -> tone_map_model_id == 1)
        {
            pitemSecond = AppendItem(pitemIf, "if( tone_map_model_id = = 1 )");

            AppendItem(pitemSecond, wxString::Format("sigmoid_midpoint = %lld", (long long)pSei->sigmoid_midpoint));
            AppendItem(pitemSecond, wxString::Format("sigmoid_width = %lld", (long long)pSei->sigmoid_width));
        }
        else if(pSei -> tone_map_model_id == 2)
        {
            pitemSecond = AppendItem(pitemIf, "if( tone_map_model_id = = 2 )");

            ploop = AppendItem(pitemSecond, "for( i = 0; i < ( 1 << target_bit_depth ); i++ )");

            for(std::size_t i = 0; i<(std::size_t)(1 << pSei->target_bit_depth); i++)
            {
                AppendItem(ploop, wxString::Format("start_of_coded_interval[%lld] = %lld", (long long)i, (long long)pSei -> start_of_coded_interval[i]));
            }

        }
        else if(pSei -> tone_map_model_id == 3)
        {
            pitemSecond = AppendItem(pitemIf, "if( tone_map_model_id = = 3 )");

            AppendItem(pitemSecond, wxString::Format("num_pivots = %lld", (long long)pSei->num_pivots));

            ploop = AppendItem(pitemSecond, "for( i = 0; i < num_pivots; i++ )");

            for(std::size_t i=0; i<pSei -> num_pivots; i++)
            {
                AppendItem(ploop, wxString::Format("coded_pivot_value[%lld] = %lld", (long long)i, (long long)pSei -> coded_pivot_value[i]));
                AppendItem(ploop, wxString::Format("target_pivot_value[%lld] = %lld", (long long)i, (long long)pSei -> target_pivot_value[i]));
            }
        }
        else if(pSei -> tone_map_model_id == 4)
        {
            pitemSecond = AppendItem(pitemIf, "if( tone_map_model_id = = 4 )");

            AppendItem(pitemSecond, wxString::Format("camera_iso_speed_idc = %lld", (long long)pSei->camera_iso_speed_idc));
            if(pSei -> camera_iso_speed_idc == 255)
            {
                pitemThird = AppendItem(pitemSecond, "if( camera_iso_speed_idc = = EXTENDED_ISO )");

                AppendItem(pitemThird, wxString::Format("camera_iso_speed_value = %lld", (long long)pSei->camera_iso_speed_value));
            }

            AppendItem(pitemSecond, wxString::Format("exposure_index_idc = %lld", (long long)pSei->exposure_index_idc));
            if(pSei -> exposure_index_idc == 255)
            {
                pitemThird = AppendItem(pitemSecond, "if( exposure_index_idc = = EXTENDED_ISO )");

                AppendItem(pitemThird, wxString::Format("exposure_index_value = %lld", (long long)pSei->exposure_index_value));
            }

            AppendItem(pitemSecond, wxString::Format("exposure_compensation_value_sign_flag = %lld", (long long)pSei->exposure_compensation_value_sign_flag));
            AppendItem(pitemSecond, wxString::Format("exposure_compensation_value_numerator = %lld", (long long)pSei->exposure_compensation_value_numerator));
            AppendItem(pitemSecond, wxString::Format("exposure_compensation_value_denom_idc = %lld", (long long)pSei->exposure_compensation_value_denom_idc));
            AppendItem(pitemSecond, wxString::Format("ref_screen_luminance_white = %lld", (long long)pSei->ref_screen_luminance_white));
            AppendItem(pitemSecond, wxString::Format("extended_range_white_level = %lld", (long long)pSei->extended_range_white_level));
            AppendItem(pitemSecond, wxString::Format("nominal_black_level_code_value = %lld", (long long)pSei->nominal_black_level_code_value));
            AppendItem(pitemSecond, wxString::Format("nominal_white_level_code_value = %lld", (long long)pSei->nominal_white_level_code_value));
            AppendItem(pitemSecond, wxString::Format("extended_white_level_code_value = %lld", (long long)pSei->extended_white_level_code_value));
        }
    }
}


void SyntaxViewer::createSOPDescription(std::shared_ptr<HEVC::SOPDescription> pSei, wxTreeItemId pItem)
{
    wxTreeItemId ploop;
    wxTreeItemId pitemIf;

    AppendItem(pItem, wxString::Format("sop_seq_parameter_set_id = %lld", (long long)pSei->sop_seq_parameter_set_id));
    AppendItem(pItem, wxString::Format("num_entries_in_sop_minus1 = %lld", (long long)pSei->num_entries_in_sop_minus1));

    ploop = AppendItem(pItem, "for( i = 0; i <= num_entries_in_sop_minus1; i++ )");

    for(std::size_t i=0; i<=pSei -> num_entries_in_sop_minus1; i++)
    {
        AppendItem(ploop, wxString::Format("sop_vcl_nut[%lld] = %lld", (long long)i, (long long)pSei -> sop_vcl_nut[i]));
        AppendItem(ploop, wxString::Format("sop_temporal_id[%lld] = %lld", (long long)i, (long long)pSei -> sop_temporal_id[i]));

        if(pSei -> sop_vcl_nut[i] != 19 && pSei -> sop_vcl_nut[i] != 20)
        {
            pitemIf = AppendItem(ploop, "if( sop_vcl_nut[ i ] != IDR_W_RADL && sop_vcl_nut[ i ] != IDR_N_LP )");

            AppendItem(pitemIf, wxString::Format("sop_short_term_rps_idx[%lld] = %lld", (long long)i, (long long)pSei -> sop_short_term_rps_idx[i]));
        }

        if(i > 0)
        {
            pitemIf = AppendItem(ploop, "if( i > 0 )");

            AppendItem(pitemIf, wxString::Format("sop_poc_delta[%lld] = %lld", (long long)i, (long long)pSei -> sop_poc_delta[i]));
        }
    }
}


void SyntaxViewer::createTemporalLevel0Index(std::shared_ptr<HEVC::TemporalLevel0Index> pSeiPayload, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("temporal_sub_layer_zero_idx = %lld", (long long)pSeiPayload->temporal_sub_layer_zero_idx));
    AppendItem(pItem, wxString::Format("irap_pic_id = %lld", (long long)pSeiPayload->irap_pic_id));
}

void SyntaxViewer::createSegmRectFramePacking(std::shared_ptr<HEVC::SegmRectFramePacking> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemIf;

    AppendItem(pItem, wxString::Format("segmented_rect_frame_packing_arrangement_cancel_flag = %lld", (long long)pSei->segmented_rect_frame_packing_arrangement_cancel_flag));

    if(!pSei -> segmented_rect_frame_packing_arrangement_cancel_flag)
    {
        pitemIf = AppendItem(pItem, "if( !segmented_rect_frame_packing_arrangement_cancel_flag )");

        AppendItem(pitemIf, wxString::Format("segmented_rect_content_interpretation_type = %lld", (long long)pSei->segmented_rect_content_interpretation_type));
        AppendItem(pitemIf, wxString::Format("segmented_rect_frame_packing_arrangement_persistence = %lld", (long long)pSei->segmented_rect_frame_packing_arrangement_persistence));
    }
}


void SyntaxViewer::createTimeCode(std::shared_ptr<HEVC::TimeCode> pSei, wxTreeItemId pItem)
{
    wxTreeItemId ploop;
    wxTreeItemId pitemIf;
    wxTreeItemId pitemSecond;
    wxTreeItemId pitemSeconds;
    wxTreeItemId pitemMinuts;
    wxTreeItemId pitemHours;

    AppendItem(pItem, wxString::Format("num_clock_ts = %lld", (long long)pSei->num_clock_ts));

    if(pSei -> num_clock_ts > 0)
    {
        ploop = AppendItem(pItem, "for( i = 0; i < num_clock_ts; i++ )");

        for(std::size_t i=0; i<pSei -> num_clock_ts; i++)
        {
            AppendItem(ploop, wxString::Format("clock_time_stamp_flag[%lld] = %lld", (long long)i, (long long)pSei -> clock_time_stamp_flag[i]));

            if(pSei -> clock_time_stamp_flag[i])
            {
                pitemIf = AppendItem(ploop, wxString::Format("if( clock_time_stamp_flag[%lld] )", (long long)i));

                AppendItem(pitemIf, wxString::Format("nuit_field_based_flag[%lld] = %lld", (long long)i, (long long)pSei -> nuit_field_based_flag[i]));
                AppendItem(pitemIf, wxString::Format("counting_type[%lld] = %lld", (long long)i, (long long)pSei -> counting_type[i]));
                AppendItem(pitemIf, wxString::Format("full_timestamp_flag[%lld] = %lld", (long long)i, (long long)pSei -> full_timestamp_flag[i]));
                AppendItem(pitemIf, wxString::Format("discontinuity_flag[%lld] = %lld", (long long)i, (long long)pSei -> discontinuity_flag[i]));
                AppendItem(pitemIf, wxString::Format("cnt_dropped_flag[%lld] = %lld", (long long)i, (long long)pSei -> cnt_dropped_flag[i]));
                AppendItem(pitemIf, wxString::Format("n_frames[%lld] = %lld", (long long)i, (long long)pSei -> n_frames[i]));


                if(pSei -> full_timestamp_flag[i])
                {
                    pitemSecond = AppendItem(pitemIf, wxString::Format("if( full_timestamp_flag[%lld] )", (long long)i));

                    AppendItem(pitemSecond, wxString::Format("seconds_value[%lld] = %lld", (long long)i, (long long)pSei -> seconds_value[i]));
                    AppendItem(pitemSecond, wxString::Format("minutes_value[%lld] = %lld", (long long)i, (long long)pSei -> minutes_value[i]));
                    AppendItem(pitemSecond, wxString::Format("hours_value[%lld] = %lld", (long long)i, (long long)pSei -> hours_value[i]));
                }
                else
                {
                    pitemSecond = AppendItem(pitemIf, wxString::Format("if( !full_timestamp_flag[%lld] )", (long long)i));

                    AppendItem(pitemSecond, wxString::Format("seconds_flag[%lld] = %lld", (long long)i, (long long)pSei -> seconds_flag[i]));

                    if(pSei -> seconds_flag[i])
                    {
                        pitemSeconds = AppendItem(pitemSecond, wxString::Format("if( seconds_flag[%lld] )", (long long)i));

                        AppendItem(pitemSeconds, wxString::Format("seconds_value[%lld] = %lld", (long long)i, (long long)pSei -> seconds_value[i]));
                        AppendItem(pitemSeconds, wxString::Format("minutes_flag[%lld] = %lld", (long long)i, (long long)pSei -> minutes_flag[i]));

                        if(pSei -> minutes_flag[i])
                        {
                            pitemMinuts = AppendItem(pitemSeconds, wxString::Format("if( minutes_flag[%lld] )", (long long)i));

                            AppendItem(pitemMinuts, wxString::Format("minutes_value[%lld] = %lld", (long long)i, (long long)pSei -> minutes_value[i]));
                            AppendItem(pitemMinuts, wxString::Format("hours_flag[%lld] = %lld", (long long)i, (long long)pSei -> hours_flag[i]));

                            if(pSei -> hours_flag[i])
                            {
                                pitemHours = AppendItem(pitemMinuts, wxString::Format("if( hours_flag[%lld] )", (long long)i));

                                AppendItem(pitemHours, wxString::Format("hours_value[%lld] = %lld", (long long)i, (long long)pSei -> hours_value[i]));
                            }
                        }
                    }
                }

                AppendItem(pitemIf, wxString::Format("time_offset_length[%lld] = %lld", (long long)i, (long long)pSei -> time_offset_length[i]));
                if(pSei -> time_offset_length[i])
                {
                    pitemSecond = AppendItem(pitemIf, wxString::Format("if( time_offset_length[%lld] )", (long long)i));

                    AppendItem(pitemSecond, wxString::Format("time_offset_value[%lld] = %lld", (long long)i, (long long)pSei -> time_offset_value[i]));
                }
            }
        }
    }
}


void SyntaxViewer::createKneeFunctionInfo(std::shared_ptr<HEVC::KneeFunctionInfo> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemIf;
    wxTreeItemId ploop;

    AppendItem(pItem, wxString::Format("knee_function_id = %lld", (long long)pSei->knee_function_id));
    AppendItem(pItem, wxString::Format("knee_function_cancel_flag = %lld", (long long)pSei->knee_function_cancel_flag));

    if(!pSei -> knee_function_cancel_flag)
    {
        pitemIf = AppendItem(pItem, "if( !knee_function_cancel_flag )");

        AppendItem(pitemIf, wxString::Format("knee_function_persistence_flag = %lld", (long long)pSei -> knee_function_persistence_flag));
        AppendItem(pitemIf, wxString::Format("input_d_range = %lld", (long long)pSei -> input_d_range));
        AppendItem(pitemIf, wxString::Format("input_disp_luminance = %lld", (long long)pSei -> input_disp_luminance));
        AppendItem(pitemIf, wxString::Format("output_d_range = %lld", (long long)pSei -> output_d_range));
        AppendItem(pitemIf, wxString::Format("output_disp_luminance = %lld", (long long)pSei -> output_disp_luminance));
        AppendItem(pitemIf, wxString::Format("num_knee_points_minus1 = %lld", (long long)pSei -> num_knee_points_minus1));

        ploop = AppendItem(pitemIf, "for( i = 0; i <= num_knee_points_minus1; i++ )");

        for(std::size_t i=0; i<=pSei -> num_knee_points_minus1; i++)
        {
            AppendItem(ploop, wxString::Format("input_knee_point[%lld] = %lld", (long long)i, (long long)pSei -> input_knee_point[i]));
            AppendItem(ploop, wxString::Format("output_knee_point[%lld] = %lld", (long long)i, (long long)pSei -> output_knee_point[i]));
        }
    }
}


void SyntaxViewer::createChromaResamplingFilterHint(std::shared_ptr<HEVC::ChromaResamplingFilterHint> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemIf;
    wxTreeItemId pitemSecond;
    wxTreeItemId ploop;

    AppendItem(pItem, wxString::Format("ver_chroma_filter_idc = %lld", (long long)pSei->ver_chroma_filter_idc));
    AppendItem(pItem, wxString::Format("hor_chroma_filter_idc = %lld", (long long)pSei->hor_chroma_filter_idc));
    AppendItem(pItem, wxString::Format("ver_filtering_field_processing_flag = %lld", (long long)pSei->ver_filtering_field_processing_flag));

    if(pSei -> ver_chroma_filter_idc == 1 || pSei -> hor_chroma_filter_idc == 1)
    {
        pitemIf = AppendItem(pItem, "if( ver_chroma_filter_idc == 1 || hor_chroma_filter_idc == 1 )");

        AppendItem(pitemIf, wxString::Format("target_format_idc = %lld", (long long)pSei -> target_format_idc));

        if(pSei -> ver_chroma_filter_idc == 1)
        {
            pitemSecond = AppendItem(pitemIf, "if( ver_chroma_filter_idc == 1 )");

            AppendItem(pitemSecond, wxString::Format("num_vertical_filters = %lld", (long long)pSei -> num_vertical_filters));

            if(pSei -> num_vertical_filters)
            {
                ploop = AppendItem(pitemSecond, "for( i = 0; i < num_vertical_filters; i++ )");

                for(std::size_t i=0; i<pSei -> num_vertical_filters; i++)
                {
                    AppendItem(ploop, wxString::Format("ver_tap_length_minus_1[%lld] = %lld", (long long)i, (long long)pSei -> ver_tap_length_minus_1[i]));

                    wxString str = wxString::Format("ver_filter_coeff[%lld] = {", (long long)i);
                    for(std::size_t j=0; j<pSei -> ver_tap_length_minus_1[i]; j++)
                    {
                        str += wxString::Format("%lld, ", (long long)pSei -> ver_filter_coeff[i][j]);
                    }
                    str += wxString::Format("%lld \n}", (long long)pSei -> ver_filter_coeff[i][pSei -> ver_tap_length_minus_1[i]]);

                    AppendItem(pitemSecond, str);
                }
            }
        }

        if(pSei -> hor_chroma_filter_idc == 1)
        {
            pitemSecond = AppendItem(pitemIf, "if( hor_chroma_filter_idc == 1 )");

            AppendItem(pitemSecond, wxString::Format("num_horizontal_filters = %lld", (long long)pSei -> num_horizontal_filters));

            if(pSei -> num_horizontal_filters)
            {
                ploop = AppendItem(pitemSecond, "for( i = 0; i < num_horizontal_filters; i++ )");

                for(std::size_t i=0; i<pSei -> num_horizontal_filters; i++)
                {
                    AppendItem(ploop, wxString::Format("hor_tap_length_minus_1[%lld] = %lld", (long long)i, (long long)pSei -> hor_tap_length_minus_1[i]));

                    wxString str = wxString::Format("hor_filter_coeff[%lld] = {", (long long)i);
                    for(std::size_t j=0; j<pSei -> hor_tap_length_minus_1[i]; j++)
                    {
                        str += wxString::Format("%lld, ", (long long)pSei -> hor_filter_coeff[i][j]);
                    }
                    str += wxString::Format("%lld \n}", (long long)pSei -> hor_filter_coeff[i][pSei -> hor_tap_length_minus_1[i]]);

                    AppendItem(pitemSecond, str);
                }
            }
        }
    }
}


void SyntaxViewer::createColourRemappingInfo(std::shared_ptr<HEVC::ColourRemappingInfo> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemIf;
    wxTreeItemId pitemIfSecond;
    wxTreeItemId ploop;
    wxTreeItemId ploopSecond;

    AppendItem(pItem, wxString::Format("colour_remap_id = %lld", (long long)pSei->colour_remap_id));
    AppendItem(pItem, wxString::Format("colour_remap_cancel_flag = %lld", (long long)pSei->colour_remap_cancel_flag));

    if(!pSei -> colour_remap_cancel_flag)
    {
        pitemIf = AppendItem(pItem, "if( !colour_remap_cancel_flag )");

        AppendItem(pitemIf, wxString::Format("colour_remap_persistence_flag = %lld", (long long)pSei -> colour_remap_persistence_flag));
        AppendItem(pitemIf, wxString::Format("colour_remap_video_signal_info_present_flag = %lld", (long long)pSei -> colour_remap_video_signal_info_present_flag));

        if(pSei -> colour_remap_video_signal_info_present_flag)
        {
            pitemIfSecond = AppendItem(pitemIf, "if( colour_remap_video_signal_info_present_flag )");

            AppendItem(pitemIfSecond, wxString::Format("colour_remap_full_range_flag = %lld", (long long)pSei -> colour_remap_full_range_flag));
            AppendItem(pitemIfSecond, wxString::Format("colour_remap_primaries = %lld", (long long)pSei -> colour_remap_primaries));
            AppendItem(pitemIfSecond, wxString::Format("colour_remap_transfer_function = %lld", (long long)pSei -> colour_remap_transfer_function));
            AppendItem(pitemIfSecond, wxString::Format("colour_remap_matrix_coefficients = %lld", (long long)pSei -> colour_remap_matrix_coefficients));
        }
        AppendItem(pitemIf, wxString::Format("colour_remap_input_bit_depth = %lld", (long long)pSei -> colour_remap_input_bit_depth));
        AppendItem(pitemIf, wxString::Format("colour_remap_bit_depth = %lld", (long long)pSei -> colour_remap_bit_depth));

        ploop = AppendItem(pitemIf, "for( i = 0; i < 3; i++ )");

        for(std::size_t i=0 ; i<3 ; i++)
        {
            AppendItem(ploop, wxString::Format("pre_lut_num_val_minus1[%lld] = %lld", (long long)i, (long long)pSei -> pre_lut_num_val_minus1[i]));

            if(pSei -> pre_lut_num_val_minus1[i] > 0)
            {
                pitemIfSecond = AppendItem(ploop, wxString::Format("if( pre_lut_num_val_minus1[%lld] > 0 )", (long long)i));

                ploopSecond = AppendItem(pitemIfSecond, wxString::Format("for( j = 0; j <= pre_lut_num_val_minus1[%lld]; j++ )", (long long)i));

                for (std::size_t j=0 ; j<=pSei -> pre_lut_num_val_minus1[i]; j++)
                {
                    AppendItem(ploopSecond, wxString::Format("pre_lut_coded_value[%lld][%lld] = %lld", (long long)i, (long long)j, (long long)pSei -> pre_lut_coded_value[i][j]));
                    AppendItem(ploopSecond, wxString::Format("pre_lut_target_value[%lld][%lld] = %lld", (long long)i, (long long)j, (long long)pSei -> pre_lut_target_value[i][j]));
                }
            }
        }

        AppendItem(pitemIf, wxString::Format("colour_remap_matrix_present_flag = %lld", (long long)pSei -> colour_remap_matrix_present_flag));

        if(pSei -> colour_remap_matrix_present_flag)
        {
            pitemIfSecond = AppendItem(pitemIf, "if( colour_remap_matrix_present_flag )");

            AppendItem(pitemIfSecond, wxString::Format("log2_matrix_denom = %lld", (long long)pSei -> log2_matrix_denom));

            ploop = AppendItem(pitemIfSecond, "for( i = 0; i < 3; i++ )");

            for (std::size_t i=0 ; i<3 ; i++)
            {
                wxString str = wxString::Format("colour_remap_coeffs[%lld] = { ", (long long)i);
                for (std::size_t j=0 ; j<2 ; j++)
                {
                    str += wxString::Format("%lld, ", (long long)pSei -> colour_remap_coeffs[i][j]);
                }
                str += wxString::Format("%lld }", (long long)pSei -> colour_remap_coeffs[i][2]);

                AppendItem(ploop, str);
            }
        }

        ploop = AppendItem(pitemIf, "for( i = 0; i < 3; i++ )");

        for(std::size_t i=0 ; i<3 ; i++)
        {
            AppendItem(ploop, wxString::Format("post_lut_num_val_minus1[%lld] = %lld", (long long)i, (long long)pSei -> post_lut_num_val_minus1[i]));

            if(pSei -> post_lut_num_val_minus1[i] > 0)
            {
                pitemIfSecond = AppendItem(ploop, wxString::Format("if( post_lut_num_val_minus1[%lld] > 0 )", (long long)i));

                ploopSecond = AppendItem(pitemIfSecond, wxString::Format("for( j = 0; j <= post_lut_num_val_minus1[%lld]; j++ )", (long long)i));

                for (std::size_t j=0 ; j<=pSei -> post_lut_num_val_minus1[i]; j++)
                {
                    AppendItem(ploopSecond, wxString::Format("post_lut_coded_value[%lld][%lld] = %lld", (long long)i, (long long)j, (long long)pSei -> post_lut_coded_value[i][j]));
                    AppendItem(ploopSecond, wxString::Format("post_lut_target_value[%lld][%lld] = %lld", (long long)i, (long long)j, (long long)pSei -> post_lut_target_value[i][j]));
                }
            }
        }
    }
}


void SyntaxViewer::createSceneInfo(std::shared_ptr<HEVC::SceneInfo> pSei, wxTreeItemId pItem)
{
    wxTreeItemId pitemIf;
    wxTreeItemId pitemIfSecond;

    AppendItem(pItem, wxString::Format("scene_info_present_flag = %lld", (long long)pSei->scene_info_present_flag));

    if(pSei -> scene_info_present_flag)
    {
        pitemIf = AppendItem(pItem, "if( scene_info_present_flag )");

        AppendItem(pitemIf, wxString::Format("prev_scene_id_valid_flag = %lld", (long long)pSei -> prev_scene_id_valid_flag));
        AppendItem(pitemIf, wxString::Format("scene_id = %lld", (long long)pSei -> scene_id));
        AppendItem(pitemIf, wxString::Format("scene_transition_type = %lld", (long long)pSei -> scene_transition_type));

        if(pSei -> scene_transition_type > 3)
        {
            pitemIfSecond = AppendItem(pitemIf, "if( scene_transition_type > 3 )");

            AppendItem(pitemIfSecond, wxString::Format("second_scene_id = %lld", (long long)pSei -> second_scene_id));
        }
    }
}

void SyntaxViewer::createProgressiveRefinementSegmentStart(std::shared_ptr<HEVC::ProgressiveRefinementSegmentStart> pSei, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("progressive_refinement_id = %lld", (long long)pSei->progressive_refinement_id));
    AppendItem(pItem, wxString::Format("pic_order_cnt_delta = %lld", (long long)pSei->pic_order_cnt_delta));
}


void SyntaxViewer::createProgressiveRefinementSegmentEnd(std::shared_ptr<HEVC::ProgressiveRefinementSegmentEnd> pSei, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("progressive_refinement_id = %lld", (long long)pSei->progressive_refinement_id));
}


void SyntaxViewer::createFullFrameSnapshot(std::shared_ptr<HEVC::FullFrameSnapshot> pSei, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("snapshot_id = %lld", (long long)pSei->snapshot_id));
}


void SyntaxViewer::createRegionRefreshInfo(std::shared_ptr<HEVC::RegionRefreshInfo> pSei, wxTreeItemId pItem)
{
    AppendItem(pItem, wxString::Format("refreshed_region_flag = %lld", (long long)pSei->refreshed_region_flag));
}

void SyntaxViewer::updateItemsState()
{
    // Recursively update state
    wxTreeItemId root = GetRootItem();
    if (root.IsOk()) {
        std::function<void(wxTreeItemId)> recurse = [&](wxTreeItemId item) {
            // Only expand if it's not the root OR if the root is not hidden
            // Since we use wxTR_HIDE_ROOT, we should never call Expand on the root item.
            if (item != root) {
                if (m_state.isActive(GetItemText(item))) {
                    Expand(item);
                }
            }
            wxTreeItemIdValue cookie;
            wxTreeItemId child = GetFirstChild(item, cookie);
            while (child.IsOk()) {
                recurse(child);
                child = GetNextChild(item, cookie);
            }
        };
        recurse(root);
    }
}

bool SyntaxViewer::SyntaxViewerState::isActive(const wxString& name) const
{
    std::map<wxString, bool>::const_iterator itr = m_state.find(name);
    if(itr != m_state.end())
        return itr -> second;

    return true;
}

void SyntaxViewer::SyntaxViewerState::setActive(const wxString& name, bool isActive)
{
    m_state[name] = isActive;
}

void SyntaxViewer::onRightClick(wxTreeEvent& event)
{
    wxTreeItemId itemId = event.GetItem();
    if (!itemId.IsOk())
        return;

    SelectItem(itemId);

    wxMenu menu;
    menu.Append(wxID_COPY, "Copy");
    
    Bind(wxEVT_MENU, [this, itemId](wxCommandEvent&) {
        copyToClipboard(GetItemText(itemId));
    }, wxID_COPY);

    PopupMenu(&menu);
}

void SyntaxViewer::onKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == 'C' && event.ControlDown())
    {
        wxTreeItemId itemId = GetSelection();
        if (itemId.IsOk())
        {
            copyToClipboard(GetItemText(itemId));
        }
    }
    else
    {
        event.Skip();
    }
}

void SyntaxViewer::copyToClipboard(const wxString& text)
{
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
}
