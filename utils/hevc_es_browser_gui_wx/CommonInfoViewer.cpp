#include "CommonInfoViewer.h"
#include <wx/config.h>
#include <ConvToString.h>

CommonInfoViewer::CommonInfoViewer(wxWindow* parent)
    : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL)
{
    AppendColumn("Offset");
    AppendColumn("Length");
    AppendColumn("Nal Type");
    AppendColumn("Temporal Layer");
    AppendColumn("Layer ID");
    AppendColumn("Info");

    readCustomData();
}

void CommonInfoViewer::onNALUnit(std::shared_ptr<HEVC::NALUnit> pNALUnit, const HEVC::Parser::Info* pInfo)
{
    NALUInfo nalUInfo;
    nalUInfo.m_pNALUnit = pNALUnit;
    nalUInfo.m_info = *pInfo;
    m_nalus.push_back(nalUInfo);

    long row = GetItemCount();
    wxString offset = wxString::Format("0x%llX (%lld)", (unsigned long long)pInfo->m_position, (long long)pInfo->m_position);
    InsertItem(row, offset);

    if (row > 0)
    {
        wxString len = wxString::Format("%lld", (long long)(pInfo->m_position - m_nalus[m_nalus.size() - 2].m_info.m_position));
        SetItem(row - 1, 1, len);
    }

    SetItem(row, 2, ConvToString::NALUnitType(pNALUnit->m_nalHeader.type));

    if (pNALUnit->m_nalHeader.type == HEVC::NAL_VPS ||
        pNALUnit->m_nalHeader.type == HEVC::NAL_SPS ||
        pNALUnit->m_nalHeader.type == HEVC::NAL_PPS)
    {
        SetItem(row, 3, "0");
        SetItem(row, 4, "0");
    }
    else
    {
        SetItem(row, 3, wxString::Format("%d", pNALUnit->m_nalHeader.temporal_id_plus1 - 1));
        SetItem(row, 4, wxString::Format("%d", pNALUnit->m_nalHeader.layer_id));
    }

    wxString info;
    switch (pNALUnit->m_nalHeader.type)
    {
        case HEVC::NAL_VPS: info = "VPS"; break;
        case HEVC::NAL_SPS: info = "SPS"; break;
        case HEVC::NAL_PPS: info = "PPS"; break;
        case HEVC::NAL_IDR_W_RADL:
        case HEVC::NAL_IDR_N_LP: info = "IDR Slice"; break;
        case HEVC::NAL_TRAIL_R:
        case HEVC::NAL_TSA_R:
        case HEVC::NAL_STSA_R:
        case HEVC::NAL_RADL_R:
        case HEVC::NAL_RASL_R:
        case HEVC::NAL_TRAIL_N:
        case HEVC::NAL_TSA_N:
        case HEVC::NAL_STSA_N:
        case HEVC::NAL_RADL_N:
        case HEVC::NAL_RASL_N:
        case HEVC::NAL_BLA_W_LP:
        case HEVC::NAL_BLA_W_RADL:
        case HEVC::NAL_BLA_N_LP:
        case HEVC::NAL_CRA_NUT:
        {
            auto pSlice = std::dynamic_pointer_cast<HEVC::Slice>(pNALUnit);
            if (pSlice->dependent_slice_segment_flag) info = "Dependent Slice";
            else
            {
                switch (pSlice->slice_type)
                {
                    case HEVC::Slice::B_SLICE: info = "B Slice"; break;
                    case HEVC::Slice::P_SLICE: info = "P Slice"; break;
                    case HEVC::Slice::I_SLICE: info = "I Slice"; break;
                }
            }
            break;
        }
        case HEVC::NAL_AUD: info = "Access unit delimiter"; break;
        case HEVC::NAL_EOS_NUT: info = "End of sequence"; break;
        case HEVC::NAL_EOB_NUT: info = "End of bitstream"; break;
        case HEVC::NAL_FD_NUT: info = "Filler data"; break;
        case HEVC::NAL_SEI_PREFIX:
        case HEVC::NAL_SEI_SUFFIX: info = "SEI"; break;
        default: break;
    }
    SetItem(row, 5, info);

    // Update maps for parameter sets (simplified emission)
    if (pNALUnit->m_nalHeader.type == HEVC::NAL_VPS) m_vpsMap[std::dynamic_pointer_cast<HEVC::VPS>(pNALUnit)->vps_video_parameter_set_id] = std::dynamic_pointer_cast<HEVC::VPS>(pNALUnit);
    else if (pNALUnit->m_nalHeader.type == HEVC::NAL_SPS) m_spsMap[std::dynamic_pointer_cast<HEVC::SPS>(pNALUnit)->sps_seq_parameter_set_id] = std::dynamic_pointer_cast<HEVC::SPS>(pNALUnit);
    else if (pNALUnit->m_nalHeader.type == HEVC::NAL_PPS) m_ppsMap[std::dynamic_pointer_cast<HEVC::PPS>(pNALUnit)->pps_pic_parameter_set_id] = std::dynamic_pointer_cast<HEVC::PPS>(pNALUnit);
}

void CommonInfoViewer::OnItemSelected(wxListEvent& event)
{
    m_selectedIdx = event.GetIndex();
}

std::pair<std::shared_ptr<HEVC::NALUnit>, ParserInfo> CommonInfoViewer::getSelectedNALUInfo()
{
    if (m_selectedIdx >= 0 && m_selectedIdx < (int)m_nalus.size())
    {
        return {m_nalus[m_selectedIdx].m_pNALUnit, m_nalus[m_selectedIdx].m_info};
    }
    return {nullptr, ParserInfo()};
}

void CommonInfoViewer::clear()
{
    DeleteAllItems();
    m_nalus.clear();
    m_selectedIdx = -1;
}

void CommonInfoViewer::saveCustomData()
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    for (int i = 0; i < 4; ++i)
        config->Write(wxString::Format("CommonInfoViewer/%dWidth", i), GetColumnWidth(i));
}

void CommonInfoViewer::readCustomData()
{
    wxConfig* config = (wxConfig*)wxConfig::Get();
    for (int i = 0; i < 4; ++i)
    {
        int w;
        if (config->Read(wxString::Format("CommonInfoViewer/%dWidth", i), &w))
            SetColumnWidth(i, w);
    }
}
