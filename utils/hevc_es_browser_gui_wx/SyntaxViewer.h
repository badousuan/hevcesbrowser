#ifndef SYNTAX_VIEWER_H_
#define SYNTAX_VIEWER_H_

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <Hevc.h>
#include <HevcParser.h>
#include <map>
#include <memory>
#include "types.h"

class SyntaxViewer : public wxTreeCtrl
{
public:
    SyntaxViewer(wxWindow* parent);

    void updateItemsState();
    void onNalUChanged(std::shared_ptr<HEVC::NALUnit> pNALUnit, ParserInfo info);
    void setParameretsSets(const VPSMap& vpsMap, const SPSMap& spsMap, const PPSMap& ppsMap);

private:
    void createVPS(std::shared_ptr<HEVC::VPS> pVPS);
    void createSPS(std::shared_ptr<HEVC::SPS> pSPS);
    void createPPS(std::shared_ptr<HEVC::PPS> pPPS);
    void createSlice(std::shared_ptr<HEVC::Slice> pSlice);
    void createAUD(std::shared_ptr<HEVC::AUD> pAUD);
    void createSEI(std::shared_ptr<HEVC::SEI> pSEI);

    void createProfileTierLevel(const HEVC::ProfileTierLevel& ptl, wxTreeItemId pItem);
    void createVuiParameters(const HEVC::VuiParameters& vui, size_t maxNumSubLayersMinus1, wxTreeItemId pItem);
    void createHrdParameters(const HEVC::HrdParameters& hrd, uint8_t commonInfPresentFlag, wxTreeItemId pItem);
    void createSubLayerHrdParameters(const HEVC::SubLayerHrdParameters& slhrd, uint8_t sub_pic_hrd_params_present_flag, wxTreeItemId pItem);
    void createShortTermRefPicSet(size_t stRpsIdx, const HEVC::ShortTermRefPicSet& rpset, size_t num_short_term_ref_pic_sets, const std::vector<HEVC::ShortTermRefPicSet>& refPicSets, wxTreeItemId pItem);
    void createScalingListData(const HEVC::ScalingListData& scdata, wxTreeItemId pItem);
    void createRefPicListModification(const HEVC::RefPicListModification& rplModif, wxTreeItemId pItem);
    void createPredWeightTable(const HEVC::PredWeightTable& pwt, std::shared_ptr<HEVC::Slice> pSlice, wxTreeItemId pItem);

    void createDecodedPictureHash(std::shared_ptr<HEVC::DecodedPictureHash> pDecPictHash, wxTreeItemId pItem);
    void createUserDataUnregistered(std::shared_ptr<HEVC::UserDataUnregistered> pSei, wxTreeItemId pItem);
    void createSceneInfo(std::shared_ptr<HEVC::SceneInfo> pSei, wxTreeItemId pItem);
    void createFullFrameSnapshot(std::shared_ptr<HEVC::FullFrameSnapshot> pSei, wxTreeItemId pItem);
    void createProgressiveRefinementSegmentStart(std::shared_ptr<HEVC::ProgressiveRefinementSegmentStart> pSeiPayload, wxTreeItemId pItem);
    void createProgressiveRefinementSegmentEnd(std::shared_ptr<HEVC::ProgressiveRefinementSegmentEnd> pSeiPayload, wxTreeItemId pItem);
    void createBufferingPeriod(std::shared_ptr<HEVC::BufferingPeriod> pSei, wxTreeItemId pItem);
    void createPicTiming(std::shared_ptr<HEVC::PicTiming> pSei, wxTreeItemId pItem);
    void createRecoveryPoint(std::shared_ptr<HEVC::RecoveryPoint> pSei, wxTreeItemId pItem);
    void createToneMapping(std::shared_ptr<HEVC::ToneMapping> pSei, wxTreeItemId pItem);
    void createFramePacking(std::shared_ptr<HEVC::FramePacking> pSei, wxTreeItemId pItem);
    void createDisplayOrientation(std::shared_ptr<HEVC::DisplayOrientation> pSei, wxTreeItemId pItem);
    void createSOPDescription(std::shared_ptr<HEVC::SOPDescription> pSei, wxTreeItemId pItem);
    void createActiveParameterSets(std::shared_ptr<HEVC::ActiveParameterSets> pSeiPayload, wxTreeItemId pItem);
    void createTemporalLevel0Index(std::shared_ptr<HEVC::TemporalLevel0Index> pSeiPayload, wxTreeItemId pItem);
    void createRegionRefreshInfo(std::shared_ptr<HEVC::RegionRefreshInfo> pSeiPayload, wxTreeItemId pItem);
    void createTimeCode(std::shared_ptr<HEVC::TimeCode> pSei, wxTreeItemId pItem);
    void createMasteringDisplayInfo(std::shared_ptr<HEVC::MasteringDisplayInfo> pSei, wxTreeItemId pItem);
    void createSegmRectFramePacking(std::shared_ptr<HEVC::SegmRectFramePacking> pSei, wxTreeItemId pItem);
    void createKneeFunctionInfo(std::shared_ptr<HEVC::KneeFunctionInfo> pSei, wxTreeItemId pItem);
    void createChromaResamplingFilterHint(std::shared_ptr<HEVC::ChromaResamplingFilterHint> pSeiPayload, wxTreeItemId pItem);
    void createColourRemappingInfo(std::shared_ptr<HEVC::ColourRemappingInfo> pSeiPayload, wxTreeItemId pItem);
    void createContentLightLevelInfo(std::shared_ptr<HEVC::ContentLightLevelInfo> pSeiPayload, wxTreeItemId pItem);
    void createAlternativeTransferCharacteristics(std::shared_ptr<HEVC::AlternativeTransferCharacteristics> pSeiPayload, wxTreeItemId pItem);

    std::map<uint32_t, std::shared_ptr<HEVC::VPS>> m_vpsMap;
    std::map<uint32_t, std::shared_ptr<HEVC::SPS>> m_spsMap;
    std::map<uint32_t, std::shared_ptr<HEVC::PPS>> m_ppsMap;

    class SyntaxViewerState
    {
    public:
        bool isActive(const wxString& name) const;
        void setActive(const wxString& name, bool isActive);
    private:
        std::map<wxString, bool> m_state;
    };

    SyntaxViewerState m_state;
};

#endif
