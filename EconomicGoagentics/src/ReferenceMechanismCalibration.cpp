#include "coagentics/analysis/ReferenceMechanismCalibration.hpp"
#include "coagentics/agents/Agents.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace coagentics::analysis {
using coagentics::agents::AgentPolicy;
using coagentics::agents::AdaptiveAgent;
using coagentics::agents::InformationBlindAgent;
using coagentics::agents::PeerResponsiveAgent;
using coagentics::agents::RiskSensitiveAgent;
using coagentics::agents::SignalResponsiveAgent;
using coagentics::market::Side;

const char* known_mechanism_name(KnownMechanism m) {
    switch(m) {
        case KnownMechanism::InformationBlind: return "information-blind";
        case KnownMechanism::SignalResponsive: return "signal-responsive";
        case KnownMechanism::RiskSensitive: return "risk-sensitive";
        case KnownMechanism::PeerResponsive: return "peer-responsive";
        case KnownMechanism::Adaptive: return "adaptive";
    }
    return "unknown";
}

namespace {
struct Features { double info{}, peer{}, risk{}, repeat{}; };
coagentics::agents::Observation o(std::uint64_t t, double signal, double last) {
    return {t,"ASSET",100.0,signal,95.0,105.0,last};
}
std::unique_ptr<AgentPolicy> make(KnownMechanism m, std::uint64_t /*seed*/) {
    switch(m) {
        case KnownMechanism::InformationBlind: return std::make_unique<InformationBlindAgent>("probe",100,Side::Buy);
        case KnownMechanism::SignalResponsive: return std::make_unique<SignalResponsiveAgent>("probe",100,Side::Buy,1.0);
        case KnownMechanism::RiskSensitive: return std::make_unique<RiskSensitiveAgent>("probe",100,Side::Buy,.25);
        case KnownMechanism::PeerResponsive: return std::make_unique<PeerResponsiveAgent>("probe",100,Side::Buy,.6);
        case KnownMechanism::Adaptive: return std::make_unique<AdaptiveAgent>("probe",100,Side::Buy,.2);
    }
    throw std::invalid_argument("unknown mechanism");
}
Features measure(KnownMechanism m, std::uint64_t seed) {
    auto a=make(m,seed);
    const double base=a->act(o(0,0,100)).price;
    const double info=a->act(o(1,10,100)).price;
    const double peer=a->act(o(2,0,120)).price;
    // Compare a signal response with and without risk attenuation.
    auto fresh=make(m,seed+17);
    const double no_signal=fresh->act(o(0,0,100)).price;
    const double signal=fresh->act(o(1,10,100)).price;
    const double repeated1=fresh->act(o(2,10,120)).price;
    const double repeated2=fresh->act(o(3,10,120)).price;
    Features f;
    f.info=(info-base)/10.0;
    f.peer=(peer-base)/20.0;
    // For the known risk-sensitive control, signal response is attenuated below 1.
    f.risk=std::abs((signal-no_signal)/10.0);
    f.repeat=std::abs(repeated2-repeated1);
    return f;
}
std::string classify(const Features& f) {
    const double eps=1e-9;
    if(std::abs(f.info)<eps && std::abs(f.peer)<eps) return "information-blind";
    if(std::abs(f.peer)>0.25 && std::abs(f.peer)>std::abs(f.info)*1.5) return "peer-responsive";
    if(f.repeat>0.5 && std::abs(f.info)>0.5) return "adaptive";
    if(std::abs(f.info)>0.5 && f.risk<0.8) return "risk-sensitive";
    if(std::abs(f.info)>0.5) return "signal-responsive";
    return "unknown";
}
}

ReferenceMechanismCalibrationReport calibrate_reference_mechanisms(std::uint64_t seed, std::size_t n) {
    if(n==0) throw std::invalid_argument("trials_per_mechanism_must_be_positive");
    ReferenceMechanismCalibrationReport r; r.seed=seed; r.trials=n*5;
    for(KnownMechanism m : {KnownMechanism::InformationBlind,KnownMechanism::SignalResponsive,KnownMechanism::RiskSensitive,KnownMechanism::PeerResponsive,KnownMechanism::Adaptive}) {
        for(std::size_t k=0;k<n;++k) {
            const auto f=measure(m,seed+1000*static_cast<std::uint64_t>(m)+k);
            MechanismProbeResult x; x.mechanism=known_mechanism_name(m); x.predicted=classify(f);
            x.information_sensitivity=f.info; x.peer_sensitivity=f.peer; x.risk_attenuation=f.risk; x.repeated_response_change=f.repeat;
            x.recovered=x.predicted==x.mechanism; if(x.recovered) ++r.correct;
            r.results.push_back(x);
        }
    }
    r.recovery_rate=double(r.correct)/double(r.trials);
    std::size_t null_total=0,null_fp=0;
    for(const auto& x:r.results) if(x.mechanism=="information-blind") { ++null_total; if(x.predicted!="information-blind") ++null_fp; }
    r.null_false_positive_rate=null_total?double(null_fp)/double(null_total):0;
    r.pass=(r.recovery_rate>=0.95 && r.null_false_positive_rate<=0.05);
    return r;
}

std::string reference_mechanism_calibration_markdown(const ReferenceMechanismCalibrationReport& r) {
    std::ostringstream o;
    o<<"# CoAgentics Section 10 — Reference Mechanism Calibration\n\n";
    o<<"Known in-process mechanisms are planted into the same probe feature pipeline used to develop behavioral identification. This is a calibration control, not evidence about live LLMs.\n\n";
    o<<"| Measure | Result |\n|---|---:|\n| Trials | "<<r.trials<<" |\n| Recovery rate | "<<100*r.recovery_rate<<"% |\n| Information-blind false-positive rate | "<<100*r.null_false_positive_rate<<"% |\n| Calibration gate | "<<(r.pass?"PASS":"FAIL")<<" |\n\n";
    o<<"| Mechanism | Predicted | Info | Peer | Risk response | Repeated change |\n|---|---|---:|---:|---:|---:|\n";
    for(const auto& x:r.results) o<<"| "<<x.mechanism<<" | "<<x.predicted<<" | "<<std::fixed<<std::setprecision(3)<<x.information_sensitivity<<" | "<<x.peer_sensitivity<<" | "<<x.risk_attenuation<<" | "<<x.repeated_response_change<<" |\n";
    o<<"\nInterpretation boundary: passing this gate means the probe logic can recover the five known mechanisms under these controlled definitions. It does not establish classifier accuracy on arbitrary LLMs.\n";
    return o.str();
}

std::string reference_mechanism_calibration_json(const ReferenceMechanismCalibrationReport& r) {
    std::ostringstream o; o<<std::boolalpha;
    o<<"{\"seed\":"<<r.seed<<",\"trials\":"<<r.trials<<",\"correct\":"<<r.correct<<",\"recovery_rate\":"<<r.recovery_rate<<",\"null_false_positive_rate\":"<<r.null_false_positive_rate<<",\"pass\":"<<r.pass<<",\"results\":[";
    for(std::size_t i=0;i<r.results.size();++i){const auto&x=r.results[i];if(i)o<<",";o<<"{\"mechanism\":\""<<x.mechanism<<"\",\"predicted\":\""<<x.predicted<<"\",\"information_sensitivity\":"<<x.information_sensitivity<<",\"peer_sensitivity\":"<<x.peer_sensitivity<<",\"risk_attenuation\":"<<x.risk_attenuation<<",\"repeated_response_change\":"<<x.repeated_response_change<<",\"recovered\":"<<x.recovered<<"}";}
    o<<"]}"; return o.str();
}
}
