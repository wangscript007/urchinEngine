#ifndef URCHINENGINE_LIGHTPANELWIDGET_H
#define URCHINENGINE_LIGHTPANELWIDGET_H

#include <string>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>

#include "UrchinCommon.h"
#include "controller/lights/LightController.h"
#include "panel/lights/LightTableView.h"

namespace urchin
{

    class LightPanelWidget : public QWidget, public Observer
    {
        Q_OBJECT

        public:
            LightPanelWidget();
            ~LightPanelWidget() override = default;

            LightTableView *getLightTableView() const;

            void load(LightController *);
            void unload();

        private:
            void setupGeneralPropertiesBox(QVBoxLayout *);
            void setupSpecificOmnidirectionalLightBox(QVBoxLayout *);
            void setupSpecificSunLightBox(QVBoxLayout *);

            void notify(Observable *, int) override;
            void setupLightDataFrom(const SceneLight *);
            void setupOmnidirectionalLightDataFrom(const OmnidirectionalLight *);
            void setupSunLightDataFrom(const SunLight *);

            LightController *lightController;

            LightTableView *lightTableView;
            QPushButton *addLightButton;
            QPushButton *removeLightButton;

            QGroupBox *generalPropertiesGroupBox;
            QGroupBox *specificOmnidirectionalLightGroupBox;
            QGroupBox *specificSunLightGroupBox;

            bool disableLightEvent;

            //general properties
            QDoubleSpinBox *ambientR, *ambientG, *ambientB;
            QCheckBox *produceShadowCheckBox;
            QLabel *lightType;

            //specific properties
            QDoubleSpinBox *positionX, *positionY, *positionZ, *attenuation;
            QDoubleSpinBox *directionX, *directionY, *directionZ;

        private slots:
            void showAddLightDialog();
            void removeSelectedLight();

            void updateLightGeneralProperties();
            void updateLightSpecificProperties();
    };

}

#endif
