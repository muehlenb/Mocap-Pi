# pragma once

#include "src/model/DataModel.h"

/**
 * Represents a processor class, which processes the data model
 * and the current state.
 *
 * Note that the model represents something like the 'long-term
 * memory' which included predefined rigid bodies, skeletons or
 * the camera, and state data represents data that is only valid
 * for one frame, e.g. the time or found markers.
 */
class Processor {
protected:
    /**
     * Reference to the global data model.
     */
    DataModel& model;

public:
    /**
     * Creates the processor and sets the global data model.
     */
    Processor(DataModel& model)
        : model(model){}


    virtual void process(TickData& state) = 0;
};
