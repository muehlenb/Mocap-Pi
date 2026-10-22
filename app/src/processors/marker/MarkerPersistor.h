#pragma once

#include <vector>
#include "src/model/DataModel.h"
#include "src/util/math/Vec4.h"

#include "src/processors/Processor.h"

class MarkerPersistor : public Processor{

public:
    MarkerPersistor(DataModel& model)
        : Processor(model){}

    virtual void process(TickData& tick) override {
        std::vector<Marker> markers;
        for(RawMarker& m : tick.markers)
            markers.push_back(m);

        model.markers = markers;
    }
};
