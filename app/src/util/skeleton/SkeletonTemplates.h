#include "src/model/skeleton/Skeleton.h"

/**
 * @brief The SkeletonTemplates class
 */

class SkeletonTemplates {
public:
    /**
     * Selected skeleton template (selected by the combobox in the GUI).
     */
    static int selectedSkeletonTemplate;

    /**
     * Names of the available skeletal templates.
     */
    static const char* skeletonTemplateNames[];

    /**
     * Number of the available skeletal templates.
     */
    static unsigned int skeletonTemplateNum;

    /**
     * Builds an Instance of the selected skeleton template.
     */
    static Skeleton buildSelectedSkeletonInstance(std::string name){
        Skeleton result;

        if(SkeletonTemplates::selectedSkeletonTemplate == 0)
            result = getXBotSkeletonTemplateA();

        result.name = name;
        return result;
    }

    /**
     * Gets a skeleton of the X-Bot (Mixamo, Dance Animation) with
     * marker configuration A.
     */
    static Skeleton getXBotSkeletonTemplateA();
};
