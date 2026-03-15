#include "FrameDependencyAnalyzer.h"
#include <iostream>

void FrameDependencyAnalyzer::onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo) {
    switch (pNALUnit->m_nalHeader.type) {
        case HEVC::NAL_SPS: {
            auto pSPS = std::dynamic_pointer_cast<HEVC::SPS>(pNALUnit);
            m_spsMap[pSPS->sps_seq_parameter_set_id] = pSPS;
            break;
        }
        case HEVC::NAL_PPS: {
            auto pPPS = std::dynamic_pointer_cast<HEVC::PPS>(pNALUnit);
            m_ppsMap[pPPS->pps_pic_parameter_set_id] = pPPS;
            break;
        }
        case HEVC::NAL_TRAIL_R:
        case HEVC::NAL_TRAIL_N:
        case HEVC::NAL_TSA_N:
        case HEVC::NAL_TSA_R:
        case HEVC::NAL_STSA_N:
        case HEVC::NAL_STSA_R:
        case HEVC::NAL_BLA_W_LP:
        case HEVC::NAL_BLA_W_RADL:
        case HEVC::NAL_BLA_N_LP:
        case HEVC::NAL_IDR_W_RADL:
        case HEVC::NAL_IDR_N_LP:
        case HEVC::NAL_CRA_NUT:
        case HEVC::NAL_RADL_N:
        case HEVC::NAL_RADL_R:
        case HEVC::NAL_RASL_N:
        case HEVC::NAL_RASL_R: {
            auto pSlice = std::dynamic_pointer_cast<HEVC::Slice>(pNALUnit);
            if (pSlice && pSlice->first_slice_segment_in_pic_flag) {
                processRPS(pSlice);
            }
            break;
        }
        default:
            break;
    }
}

void FrameDependencyAnalyzer::processRPS(std::shared_ptr<HEVC::Slice> pSlice) {
    auto pps_it = m_ppsMap.find(pSlice->slice_pic_parameter_set_id);
    if (pps_it == m_ppsMap.end()) return;
    auto pPPS = pps_it->second;

    auto sps_it = m_spsMap.find(pPPS->pps_seq_parameter_set_id);
    if (sps_it == m_spsMap.end()) return;
    auto pSPS = sps_it->second;

    int max_poc_lsb = 1 << (pSPS->log2_max_pic_order_cnt_lsb_minus4 + 4);
    int poc_lsb = pSlice->slice_pic_order_cnt_lsb;

    int currentPoc;
    if (pSlice->m_nalHeader.type == HEVC::NAL_IDR_W_RADL || pSlice->m_nalHeader.type == HEVC::NAL_IDR_N_LP) {
        currentPoc = 0;
    } else {
        int prev_poc_lsb = m_lastPoc % max_poc_lsb;
        int prev_poc_msb = m_lastPoc - prev_poc_lsb;

        if ((poc_lsb < prev_poc_lsb) && ((prev_poc_lsb - poc_lsb) >= (max_poc_lsb / 2))) {
            currentPoc = prev_poc_msb + max_poc_lsb + poc_lsb;
        } else if ((poc_lsb > prev_poc_lsb) && ((poc_lsb - prev_poc_lsb) > (max_poc_lsb / 2))) {
            currentPoc = prev_poc_msb - max_poc_lsb + poc_lsb;
        } else {
            currentPoc = prev_poc_msb + poc_lsb;
        }
    }
    m_lastPoc = currentPoc;

    const HEVC::ShortTermRefPicSet* rps = nullptr;
    if (pSlice->short_term_ref_pic_set_sps_flag) {
        if (pSlice->short_term_ref_pic_set_idx < pSPS->short_term_ref_pic_set.size()) {
            rps = &pSPS->short_term_ref_pic_set[pSlice->short_term_ref_pic_set_idx];
        }
    } else {
        rps = &pSlice->short_term_ref_pic_set;
    }

    if (!rps) return;

    // IMPORTANT FIX: If inter_ref_pic_set_prediction_flag is set, the RPS is predicted
    // from another RPS. The parser may not fill num_negative_pics and num_positive_pics,
    // leading to garbage values and a crash. We skip these for now.
    if (rps->inter_ref_pic_set_prediction_flag) {
        std::cout << "Skipping predicted RPS for frame " << currentPoc << " (not implemented)." << std::endl;
        return;
    }

    std::set<int> dependencies;

    // Negative POCs (Delta POCs are negative)
    for (uint32_t i = 0; i < rps->num_negative_pics; ++i) {
        if (i < rps->used_by_curr_pic_s0_flag.size() && rps->used_by_curr_pic_s0_flag[i]) {
            int poc = currentPoc - (rps->delta_poc_s0_minus1[i] + 1);
            dependencies.insert(poc);
        }
    }

    // Positive POCs (Delta POCs are positive)
    for (uint32_t i = 0; i < rps->num_positive_pics; ++i) {
        if (i < rps->used_by_curr_pic_s1_flag.size() && rps->used_by_curr_pic_s1_flag[i]) {
            int poc = currentPoc + (rps->delta_poc_s1_minus1[i] + 1);
            dependencies.insert(poc);
        }
    }

    // NOTE: Long-term references are not handled here.

    m_dependencies[currentPoc] = dependencies;
}

void FrameDependencyAnalyzer::writeDependencies(std::ostream& out) {
    out << "Frame Dependencies (POC -> depends on POCs):" << std::endl;
    for (const auto& pair : m_dependencies) {
        out << "Frame " << pair.first << " -> { ";
        for (int poc : pair.second) {
            out << poc << " ";
        }
        out << "}" << std::endl;
    }
}
