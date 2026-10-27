#include "OptionsView.h"
#include "Format.h"
#include "OptionsController.h"
#include "OptionsModel.h"
#include "common/clipboard/Clipboard.h"
#include "common/platform/Platform.h"
#include "graphics/Graphics.h"
#include "graphics/Renderer.h"
#include "gui/Style.h"
#include "simulation/ElementDefs.h"
#include "simulation/SimulationSettings.h"
#include "simulation/Air.h"
#include "client/Client.h"
#include "gui/credits/Credits.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/InformationMessage.h"
#include "gui/interface/Button.h"
#include "gui/interface/Checkbox.h"
#include "gui/interface/DropDown.h"
#include "gui/interface/Engine.h"
#include "gui/interface/Label.h"
#include "gui/interface/Separator.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/DirectionSelector.h"
#include "PowderToySDL.h"
#include "Config.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <SDL.h>

class DirectionSelector : public ui::Window
{
	std::function<void (float, float)> done;

	void OnTryExit(ExitMethod method) override
	{
		CloseActiveWindow();
		SelfDestruct();
	}

	void OnDraw() override
	{
		Graphics * g = GetGraphics();

		g->DrawFilledRect(RectSized(Position - Vec2{ 1, 1 }, Size + Vec2{ 2, 2 }), 0x000000_rgb);
		g->DrawRect(RectSized(Position, Size), 0xC8C8C8_rgb);
	}

	ui::DirectionSelector * direction;
	ui::Label * labelValues;

public:
	DirectionSelector(ui::Point position, float scale, int radius, float x, float y, String label, std::function<void (float, float)> newDone):
		ui::Window(position, ui::Point((radius * 5 / 2) + 20, (radius * 5 / 2) + 75)),
		done(newDone),
		direction(new ui::DirectionSelector(ui::Point(10, 32), scale, radius, radius / 4, 2, 5))
	{
		ui::Label * tempLabel = new ui::Label(ui::Point(4, 1), ui::Point(Size.X - 8, 22), label);
		tempLabel->SetTextColour(style::Colour::InformationTitle);
		tempLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		tempLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		AddComponent(tempLabel);

		auto * tempSeparator = new ui::Separator(ui::Point(0, 22), ui::Point(Size.X, 1));
		AddComponent(tempSeparator);

		labelValues = new ui::Label(ui::Point(0, (radius * 5 / 2) + 37), ui::Point(Size.X, 16), String::Build(Format::Precision(1), "X:", x, " Y:", y, " Total:", std::hypot(x, y)));
		labelValues->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
		labelValues->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		AddComponent(labelValues);

		direction->SetValues(x, y);
		direction->SetUpdateCallback([this](float x, float y) {
			labelValues->SetText(String::Build(Format::Precision(1), "X:", x, " Y:", y, " Total:", std::hypot(x, y)));
		});
		direction->SetSnapPoints(5, 5, 2);
		AddComponent(direction);

		ui::Button * okayButton = new ui::Button(ui::Point(0, Size.Y - 17), ui::Point(Size.X, 17), "OK");
		okayButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
		okayButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		okayButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
		okayButton->SetActionCallback({ [this] {
			done(direction->GetXValue(), direction->GetYValue());
			CloseActiveWindow();
			SelfDestruct();
		} });
		AddComponent(okayButton);
		SetOkayButton(okayButton);

		MakeActiveWindow();
	}
};

namespace
{
	enum FpsLimitDropdown
	{
		fpsLimitDropdownExact,
		fpsLimitDropdownUncapped,
	};
	enum DrawLimitDropdown
	{
		drawLimitDropdownExact,
		drawLimitDropdownFollowDisplay,
	};
};

OptionsView::OptionsView() : ui::Window(ui::Point(-1, -1), ui::Point(320, 340))
{
	auto autoWidth = [this](ui::Component *c, int extra) {
		c->Size.X = Size.X - c->Position.X - 12 - extra;
	};
	
	{
		auto *label = new ui::Label(ui::Point(4, 1), ui::Point(Size.X-8, 22), "Настройки");
		label->SetTextColour(style::Colour::InformationTitle);
		label->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		autoWidth(label, 0);
		AddComponent(label);
	}

	auto *tmpSeparator = new ui::Separator(ui::Point(0, 22), ui::Point(Size.X, 1));
	AddComponent(tmpSeparator);

	scrollPanel = new ui::ScrollPanel(ui::Point(1, 23), ui::Point(Size.X-2, Size.Y-39));
	
	AddComponent(scrollPanel);

	int currentY = 8;
	auto addLabel = [this, &currentY, &autoWidth](int indent, String text) {
		auto *label = new ui::Label(ui::Point(22 + indent * 15, currentY), ui::Point(1, 16), "");
		autoWidth(label, 0);
		label->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		label->SetMultiline(true);
		label->SetText("\bg" + text); // stupid hack because autoWidth just changes Size.X and that doesn't update the text wrapper
		label->AutoHeight();
		scrollPanel->AddChild(label);
		currentY += label->Size.Y - 1;
		return label;
	};
	auto addCheckbox = [this, &currentY, &autoWidth, &addLabel](int indent, String text, String info, std::function<void ()> action) {
		auto *checkbox = new ui::Checkbox(ui::Point(8 + indent * 15, currentY), ui::Point(1, 16), text, "");
		autoWidth(checkbox, 0);
		checkbox->SetActionCallback({ action });
		currentY += 14;
		if (info.size())
		{
			addLabel(indent, info);
		}
		currentY += 4;
		scrollPanel->AddChild(checkbox);
		return checkbox;
	};
	auto addDropDown = [this, &currentY, &autoWidth](String info, std::vector<std::pair<String, int>> options, std::function<void ()> action) {
		auto *dropDown = new ui::DropDown(ui::Point(Size.X - 95, currentY), ui::Point(80, 16));
		scrollPanel->AddChild(dropDown);
		for (auto &option : options)
		{
			dropDown->AddOption(option);
		}
		dropDown->SetActionCallback({ action });
		auto *label = new ui::Label(ui::Point(8, currentY), ui::Point(Size.X - 96, 16), info);
		label->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		scrollPanel->AddChild(label);
		autoWidth(label, 85);
		currentY += 20;
		return dropDown;
	};
	auto addButtonWithLabel = [this, &currentY, &addLabel](String buttonText, String labelText, std::function<void ()> action) {
		auto *button = new ui::Button(ui::Point(10, currentY), ui::Point(90, 16), buttonText);
		button->SetActionCallback({ action });
		scrollPanel->AddChild(button);
		auto *label = addLabel(5, labelText);
		currentY += 13;
		return label;
	};
	auto addLimitDropDown = [this, &currentY, &autoWidth](String info, std::vector<std::pair<String, int>> options, std::function<void (bool)> action) {
		auto *dropDown = new ui::DropDown(ui::Point(Size.X - 155, currentY), ui::Point(100, 16));
		scrollPanel->AddChild(dropDown);
		for (auto &option : options)
		{
			dropDown->AddOption(option);
		}
		dropDown->SetActionCallback({ [action]() {
			action(true);
		} });
		auto *textbox = new ui::Textbox(ui::Point(Size.X - 51, currentY), ui::Point(36, 16));
		textbox->SetActionCallback({ [action]() {
			action(false);
		} });
		textbox->SetDefocusCallback({ [action]() {
			action(true);
		} });
		textbox->SetLimit(4);
		scrollPanel->AddChild(textbox);
		auto *label = new ui::Label(ui::Point(8, currentY), ui::Point(Size.X - 96, 16), info);
		label->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		scrollPanel->AddChild(label);
		autoWidth(label, 105);
		currentY += 20;
		return std::make_pair(dropDown, textbox);
	};
	auto addSeparator = [this, &currentY]() {
		currentY += 6;
		auto *separator = new ui::Separator(ui::Point(0, currentY), ui::Point(Size.X, 1));
		scrollPanel->AddChild(separator);
		currentY += 11;
	};
	auto addTextboxWithPreview = [this, &currentY](String info, bool addPreview, std::function<void (String value, bool defocus)> updateFunc) {
		auto *textbox = new ui::Textbox(ui::Point(Size.X-95, currentY), ui::Point(80, 16));
		textbox->SetActionCallback({ [textbox, updateFunc] {
			updateFunc(textbox->GetText(), false);
		} });
		textbox->SetDefocusCallback({ [textbox, updateFunc] {
			updateFunc(textbox->GetText(), true);
		} });
		textbox->SetLimit(9);
		scrollPanel->AddChild(textbox);
		ui::Button *preview{};
		if (addPreview)
		{
			textbox->Size.X -= 20;
			preview = new ui::Button(ui::Point(Size.X-31, currentY), ui::Point(16, 16), "", "Предпросмотр");
			scrollPanel->AddChild(preview);
		}
		auto *label = new ui::Label(ui::Point(8, currentY), ui::Point(Size.X-105, 16), info);
		label->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		scrollPanel->AddChild(label);
		currentY += 20;
		return std::make_pair(textbox, preview);
	};

	heatSimulation = addCheckbox(0, "Теплосимуляция \bgПоявилась в версии 34", "Может вызывать странное поведение при отключении", [this] {
		c->SetHeatSimulation(heatSimulation->GetChecked());
	});
	newtonianGravity = addCheckbox(0, "Ньютоновская гравитация \bgПоявилась в версии 48", "Может снижать производительность на старых компьютерах", [this] {
		c->SetNewtonianGravity(newtonianGravity->GetChecked());
	});
	ambientHeatSimulation = addCheckbox(0, "Симуляция фонового тепла \bgПоявилась в версии 50", "Может вызывать странное/сломанное поведение со многими сохранениями", [this] {
		c->SetAmbientHeatSimulation(ambientHeatSimulation->GetChecked());
	});
	waterEqualisation = addCheckbox(0, "Выравнивание воды \bgПоявилось в версии 61", "Может снижать производительность при большом количестве воды", [this] {
		c->SetWaterEqualisation(waterEqualisation->GetChecked());
	});
	airMode = addDropDown("Режим симуляции воздуха", {
		{ "Вкл.", AIR_ON },
		{ "Давление выкл.", AIR_PRESSUREOFF },
		{ "Скорость выкл.", AIR_VELOCITYOFF },
		{ "Выкл.", AIR_OFF },
		{ "Без обновления", AIR_NOUPDATE },
	}, [this] {
		c->SetAirMode(airMode->GetOption().second);
	});
	std::tie(ambientAirTemp, ambientAirTempPreview) = addTextboxWithPreview("Температура окружающего воздуха", true, [this](String value, bool defocus) {
		UpdateAirTemp(value, defocus);
	});
	std::tie(edgePressure, edgePressurePreview) = addTextboxWithPreview("Давление окружающего воздуха", true, [this](String value, bool defocus) {
		UpdateEdgePressure(value, defocus);
	});
	{
		edgeVelocityChange = new ui::Button(ui::Point(Size.X-95, currentY), ui::Point(80, 16), "Изменить");
		scrollPanel->AddChild(edgeVelocityChange);
		edgeVelocityChange->SetActionCallback({ [this] {
			new DirectionSelector(ui::Point(-1, -1), 0.05f, 40, edgeVelocityX, edgeVelocityY, "Скорость окружающего воздуха", [this](float x, float y) {
				c->SetEdgeVelocityX(x);
				c->SetEdgeVelocityY(y);
			});
		} });
		auto *label = new ui::Label(ui::Point(8, currentY), ui::Point(Size.X-96, 16), "Скорость окружающего воздуха");
		label->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
		label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		scrollPanel->AddChild(label);
		currentY+=20;
	}
	vorticityCoeff = addTextboxWithPreview("Ограничение завихрённости", false, [this](String value, bool defocus) {
		UpdateVorticityCoeff(value, defocus);
	}).first;
	convectionMode = addDropDown("Режим конвекции тепла воздуха", {
		{ "Нет", AIRC_NONE },
		{ "Устаревший", AIRC_LEGACY },
		{ "Буссинеск", AIRC_BOUSSINESQ },
	}, [this] {
		c->SetConvectionMode(convectionMode->GetOption().second);
	});
	gravityMode = addDropDown("Режим симуляции гравитации", {
		{ "Вертикальная", GRAV_VERTICAL },
		{ "Выкл.", GRAV_OFF },
		{ "Радиальная", GRAV_RADIAL },
		{ "Пользовательская", GRAV_CUSTOM },
	}, [this] {
		c->SetGravityMode(gravityMode->GetOption().second);
		if (gravityMode->GetOption().second == 3)
		{
			new DirectionSelector(ui::Point(-1, -1), 0.05f, 40, customGravityX, customGravityY, "Пользовательская гравитация", [this](float x, float y) {
				c->SetCustomGravityX(x);
				c->SetCustomGravityY(y);
			});
		}
	});
	edgeMode = addDropDown("Режим границ", {
		{ "Пустота", EDGE_VOID },
		{ "Твёрдая", EDGE_SOLID },
		{ "Замкнутая", EDGE_LOOP },
	}, [this] {
		c->SetEdgeMode(edgeMode->GetOption().second);
	});
	temperatureScale = addDropDown("Шкала температуры", {
		{ "Кельвин", TEMPSCALE_KELVIN },
		{ "Цельсий", TEMPSCALE_CELSIUS },
		{ "Фаренгейт", TEMPSCALE_FAHRENHEIT },
	}, [this] {
		c->SetTemperatureScale(TempScale(temperatureScale->GetOption().second));
	});
	addSeparator();
	std::tie(fpsLimit, fpsLimitText) = addLimitDropDown("Ограничение частоты симуляции", {
		{ "Точно", fpsLimitDropdownExact },
		{ "Без ограничения", fpsLimitDropdownUncapped },
	}, [this](bool defocus) {
		UpdateFpsLimit(defocus);
	});
	std::tie(drawLimit, drawLimitText) = addLimitDropDown("Ограничение частоты отрисовки", {
		{ "Точно", drawLimitDropdownExact },
		{ "По дисплею", drawLimitDropdownFollowDisplay },
	}, [this](bool defocus) {
		UpdateDrawLimit(defocus);
	});
	addButtonWithLabel("Сброс", " - Установить оба лимита в разумные значения", [this]{
		c->SetFpsLimit(DefaultFpsLimit);
		c->SetDrawLimit(DefaultDrawLimit);
	});
	currentY -= 6;
	addSeparator();
	if (FORCE_WINDOW_FRAME_OPS != forceWindowFrameOpsHandheld)
	{
		std::vector<std::pair<String, int>> options;
		int currentScale = ui::Engine::Ref().GetScale();
		int scaleIndex = 1;
		bool currentScaleValid = false;
		do
		{
			if (currentScale == scaleIndex)
			{
				currentScaleValid = true;
			}
			options.push_back({ String::Build(scaleIndex), scaleIndex });
			scaleIndex += 1;
		}
		while (desktopWidth >= GetGraphics()->Size().X * scaleIndex && desktopHeight >= GetGraphics()->Size().Y * scaleIndex);
		if (!currentScaleValid)
		{
			options.push_back({ "текущий", currentScale });
		}
		scale = addDropDown("Масштаб окна для больших экранов", options, [this] {
			c->SetScale(scale->GetOption().second);
		});
	}
	if (FORCE_WINDOW_FRAME_OPS == forceWindowFrameOpsNone)
	{
		resizable = addCheckbox(0, "Изменяемый размер \bg- разрешить изменение размера и разворачивание окна", "", [this] {
			c->SetResizable(resizable->GetChecked());
		});
		fullscreen = addCheckbox(0, "Полноэкранный режим \bg- занять весь экран", "", [this] {
			c->SetFullscreen(fullscreen->GetChecked());
		});
		changeResolution = addCheckbox(1, "Установить оптимальное разрешение экрана", "", [this] {
			c->SetChangeResolution(changeResolution->GetChecked());
		});
		forceIntegerScaling = addCheckbox(1, "Целочисленный масштаб \bg- менее размыто", "", [this] {
			c->SetForceIntegerScaling(forceIntegerScaling->GetChecked());
		});
	}
	blurryScaling = addCheckbox(0, "Размытый масштаб \bg- более размыто, лучше для очень больших экранов", "", [this] {
		c->SetBlurryScaling(blurryScaling->GetChecked());
	});
	addSeparator();
	if (ALLOW_QUIT)
	{
		fastquit = addCheckbox(0, "Быстрый выход", "Полностью закрывать игру при нажатии на крестик", [this] {
			c->SetFastQuit(fastquit->GetChecked());
		});
		globalQuit = addCheckbox(0, "Глобальная комбинация выхода", "Ctrl+q работает везде", [this] {
			c->SetGlobalQuit(globalQuit->GetChecked());
		});
	}
	showAvatars = addCheckbox(0, "Показывать аватары", "Отключите, если у вас медленное соединение", [this] {
		c->SetShowAvatars(showAvatars->GetChecked());
	});
	momentumScroll = addCheckbox(0, "Инерционная (старая) прокрутка", "С ускорением вместо пошаговой прокрутки", [this] {
		c->SetMomentumScroll(momentumScroll->GetChecked());
	});
	mouseClickRequired = addCheckbox(0, "Липкие категории", "Переключать категории по клику", [this] {
		c->SetMouseClickrequired(mouseClickRequired->GetChecked());
	});
	includePressure = addCheckbox(0, "Включать давление", "При сохранении, копировании, создании штампов и т.д.", [this] {
		c->SetIncludePressure(includePressure->GetChecked());
	});
	perfectCircle = addCheckbox(0, "Идеальная круглая кисть", "Более качественная круглая кисть, без неправильных точек по краям", [this] {
		c->SetPerfectCircle(perfectCircle->GetChecked());
	});
	graveExitsConsole = addCheckbox(0, "Клавиша под Esc закрывает консоль", "Отключите, если эта клавиша на вашей клавиатуре выдаёт 0", [this] {
		c->SetGraveExitsConsole(graveExitsConsole->GetChecked());
	});
	if constexpr (PLATFORM_CLIPBOARD)
	{
		auto indent = 0;
		nativeClipoard = addCheckbox(indent, "Использовать системный буфер обмена", "Позволяет копировать и вставлять между экземплярами TPT", [this] {
			c->SetNativeClipoard(nativeClipoard->GetChecked());
		});
		currentY -= 4; // temporarily undo the currentY += 4 at the end of addCheckbox
		if (auto extra = Clipboard::Explanation())
		{
			addLabel(indent, "\bg" + *extra);
		}
		currentY += 4; // and then undo the undo
	}
	threadedRendering = addCheckbox(0, "Отдельный поток рендеринга", "Может повысить частоту кадров при использовании сложных эффектов", [this] {
		c->SetThreadedRendering(threadedRendering->GetChecked());
	});
	decoSpace = addDropDown("Цветовое пространство инструментов декораций", {
		{ "sRGB", DECOSPACE_SRGB },
		{ "Linear", DECOSPACE_LINEAR },
		{ "Gamma 2.2", DECOSPACE_GAMMA22 },
		{ "Gamma 1.8", DECOSPACE_GAMMA18 },
	}, [this] {
		c->SetDecoSpace(decoSpace->GetOption().second);
	});

	currentY += 4;
	if constexpr (ALLOW_DATA_FOLDER)
	{
		auto *dataFolderButton = new ui::Button(ui::Point(10, currentY), ui::Point(90, 16), "Открыть папку данных");
		dataFolderButton->SetActionCallback({ [] {
			ByteString cwd = Platform::GetCwd();
			if (!cwd.empty())
			{
				Platform::OpenURI(cwd);
			}
			else
			{
				std::cerr << "Не удалось открыть папку данных: Platform::GetCwd(...) failed" << std::endl;
			}
		} });
		scrollPanel->AddChild(dataFolderButton);
		if constexpr (SHARED_DATA_FOLDER)
		{
			auto *migrationButton = new ui::Button(ui::Point(Size.X - 178, currentY), ui::Point(163, 16), "Перенести в общую папку данных");
			migrationButton->SetActionCallback({ [] {
				ByteString from = Platform::originalCwd;
				ByteString to = Platform::sharedCwd;
				new ConfirmPrompt("Выполнить перенос?", "Это перенесёт все штампы, сохранения и скрипты из\n\bt" + from.FromUtf8() + "\bw\nв общую папку данных по адресу\n\bt" + to.FromUtf8() + "\bw\n\n" + "Существующие файлы не будут перезаписаны.", { [from, to]() {
						String ret = Client::Ref().DoMigration(from, to);
						new InformationMessage("Перенос завершён", ret, false);
				} });
			} });
			scrollPanel->AddChild(migrationButton);
		}
		currentY += 26;
	}
	String autoStartupRequestNote = "Выполняется один раз при запуске";
	if (!IGNORE_UPDATES)
	{
		autoStartupRequestNote += ", также проверяет обновления";
	}
	autoStartupRequest = addCheckbox(0, "Получать сообщение дня и уведомления", autoStartupRequestNote, [this] {
		auto checked = autoStartupRequest->GetChecked();
		if (checked)
		{
			Client::Ref().BeginStartupRequest();
		}
		c->SetAutoStartupRequest(checked);
	});
	startupRequestStatus = addButtonWithLabel("Получить сейчас", "", []{
		Client::Ref().BeginStartupRequest();
	});
	UpdateStartupRequestStatus();
	redirectStd = addCheckbox(0, "Сохранять ошибки и другие сообщения в файл", "Разработчики могут попросить об этом при поиске неисправностей", [this] {
		c->SetRedirectStd(redirectStd->GetChecked());
	});
	addSeparator();
	addButtonWithLabel("Авторы", " - Узнать, кто внёс вклад в TPT", []{
		auto *credits = new Credits();
		ui::Engine::Ref().ShowWindow(credits);
	});

	{
		ui::Button *ok = new ui::Button(ui::Point(0, Size.Y-16), ui::Point(Size.X, 16), "OK");
		ok->SetActionCallback({ [this] {
			c->Exit();
		} });
		AddComponent(ok);
		SetCancelButton(ok);
		SetOkayButton(ok);
	}
	scrollPanel->InnerSize = ui::Point(Size.X, currentY);
}

void OptionsView::UpdateAmbientAirTempPreview(float airTemp, bool isValid)
{
	if (isValid)
	{
		ambientAirTempPreview->Appearance.BackgroundInactive = HeatToColour(airTemp, MIN_TEMP, MAX_TEMP).WithAlpha(0xFF);
		ambientAirTempPreview->SetText("");
	}
	else
	{
		ambientAirTempPreview->Appearance.BackgroundInactive = ui::Colour(0, 0, 0);
		ambientAirTempPreview->SetText("?");
	}
	ambientAirTempPreview->Appearance.BackgroundHover = ambientAirTempPreview->Appearance.BackgroundInactive;
}

void OptionsView::AmbientAirTempToTextBox(float airTemp)
{
	StringBuilder sb;
	sb << Format::Precision(2);
	format::RenderTemperature(sb, airTemp, TempScale(temperatureScale->GetOption().second));
	ambientAirTemp->SetText(sb.Build());
}

void OptionsView::UpdateEdgePressurePreview(float edgePres, bool isValid)
{
	if (isValid)
	{
		edgePressurePreview->Appearance.BackgroundInactive = PressureToColour(edgePres).WithAlpha(0xFF);
		edgePressurePreview->SetText("");
	}
	else
	{
		edgePressurePreview->Appearance.BackgroundInactive = ui::Colour(0, 0, 0);
		edgePressurePreview->SetText("?");
	}
	edgePressurePreview->Appearance.BackgroundHover = edgePressurePreview->Appearance.BackgroundInactive;
}

void OptionsView::EdgePressureToTextBox(float edgePres)
{
	StringBuilder sb;
	sb << Format::Precision(2) << edgePres;
	edgePressure->SetText(sb.Build());
}

void OptionsView::VorticityCoeffToTextBox(float vorticity)
{
	StringBuilder sb;
	sb << Format::Precision(2) << vorticity;
	vorticityCoeff->SetText(sb.Build());
}

void OptionsView::UpdateStartupRequestStatus()
{
	switch (Client::Ref().GetStartupRequestStatus())
	{
	case Client::StartupRequestStatus::notYetDone:
		startupRequestStatus->SetText("\bg - Ещё не получено");
		break;

	case Client::StartupRequestStatus::inProgress:
		startupRequestStatus->SetText("\bg - Выполняется...");
		break;

	case Client::StartupRequestStatus::succeeded:
		startupRequestStatus->SetText(String::Build("\bg - OK, ", Client::Ref().GetServerNotifications().size(), " уведомлений получено"));
		break;

	case Client::StartupRequestStatus::failed:
		{
			auto error = Client::Ref().GetStartupRequestError();
			if (!error)
			{
				error = "???";
			}
			startupRequestStatus->SetText("\bg - Ошибка: " + error->FromUtf8());
		}
		break;
	}
}

template<class Value>
static void UpdateSettingFromString(String pres, bool isDefocus, Value minVale, Value maxValue, Value defaultValue, auto parse, auto toTextbox, auto apply)
{
	// Parse value and determine validity
	Value value = Value(0);
	bool isValid;
	try
	{
		value = parse(pres);
		isValid = true;
	}
	catch (const std::exception &ex)
	{
		isValid = false;
	}

	// While defocusing, correct out of range values and empty textboxes
	if (isDefocus)
	{
		if (pres.empty())
		{
			isValid = true;
			value = defaultValue;
		}
		else if (!isValid)
			return;
		else if (value < minVale)
			value = minVale;
		else if (value > maxValue)
			value = maxValue;

		toTextbox(value);
	}
	// Out of range values are invalid, preview should go away
	else if (isValid && (value < minVale || value > maxValue))
		isValid = false;

	// If valid, apply
	apply(value, isValid);
}

void OptionsView::UpdateFpsLimit(bool isDefocus)
{
	if (fpsLimit->GetOption().second == fpsLimitDropdownExact)
	{
		UpdateSettingFromString(fpsLimitText->GetText(), isDefocus, FpsLimitExplicit::minSane, FpsLimitExplicit::maxSane, DefaultFpsLimit.value, [](const String &s) {
			return s.ToNumber<float>();
		}, [this](float value) {
			FpsLimitToInterface(FpsLimitExplicit{ value });
		}, [this](float value, bool isValid) {
			if (isValid)
			{
				c->SetFpsLimit(FpsLimitExplicit{ value });
			}
		});
	}
	else
	{
		fpsLimitText->Enabled = false;
		fpsLimitText->SetText("");
		c->SetFpsLimit(FpsLimitNone{});
	}
}

void OptionsView::FpsLimitToInterface(SimFpsLimit limit)
{
	if (auto *fpsLimitExplicit = std::get_if<FpsLimitExplicit>(&limit))
	{
		StringBuilder sb;
		sb << fpsLimitExplicit->value;
		fpsLimitText->Enabled = true;
		fpsLimitText->SetText(sb.Build());
		fpsLimit->SetOption(fpsLimitDropdownExact);
	}
	else
	{
		fpsLimitText->Enabled = false;
		fpsLimitText->SetText("");
		fpsLimit->SetOption(fpsLimitDropdownUncapped);
	}
}

void OptionsView::UpdateDrawLimit(bool isDefocus)
{
	if (drawLimit->GetOption().second == drawLimitDropdownExact)
	{
		UpdateSettingFromString(drawLimitText->GetText(), isDefocus, DrawLimitExplicit::minSane, DrawLimitExplicit::maxSane, DefaultDrawLimit.value, [](const String &s) {
			return s.ToNumber<int>();
		}, [this](int value) {
			DrawLimitToInterface(DrawLimitExplicit{ value });
		}, [this](int value, bool isValid) {
			if (isValid)
			{
				c->SetDrawLimit(DrawLimitExplicit{ value });
			}
		});
	}
	else
	{
		drawLimitText->Enabled = false;
		drawLimitText->SetText("");
		c->SetDrawLimit(DrawLimitDisplay{});
	}
}

void OptionsView::DrawLimitToInterface(DrawLimit limit)
{
	if (auto *drawLimitExplicit = std::get_if<DrawLimitExplicit>(&limit))
	{
		StringBuilder sb;
		sb << drawLimitExplicit->value;
		drawLimitText->Enabled = true;
		drawLimitText->SetText(sb.Build());
		drawLimit->SetOption(drawLimitDropdownExact);
	}
	else
	{
		drawLimitText->Enabled = false;
		drawLimitText->SetText("");
		drawLimit->SetOption(drawLimitDropdownFollowDisplay);
	}
}

void OptionsView::UpdateAirTemp(String temp, bool isDefocus)
{
	UpdateSettingFromString(temp, isDefocus, MIN_TEMP, MAX_TEMP, float(R_TEMP) + 273.15f, [this](const String &temp) {
		return format::StringToTemperature(temp, TempScale(temperatureScale->GetOption().second));
	}, [this](float airTemp) {
		AmbientAirTempToTextBox(airTemp);
	}, [this](float airTemp, bool isValid) {
		if (isValid)
		{
			c->SetAmbientAirTemperature(airTemp);
		}
		UpdateAmbientAirTempPreview(airTemp, isValid);
	});
}

void OptionsView::UpdateEdgePressure(String pres, bool isDefocus)
{
	UpdateSettingFromString(pres, isDefocus, MIN_PRESSURE, MAX_PRESSURE, 0.f, [](const String &pres) {
		return pres.ToNumber<float>();
	}, [this](float edgePres) {
		EdgePressureToTextBox(edgePres);
	}, [this](float edgePres, bool isValid) {
		if (isValid)
		{
			c->SetEdgePressure(edgePres);
		}
		UpdateEdgePressurePreview(edgePres, isValid);
	});
}

void OptionsView::UpdateVorticityCoeff(String vort, bool isDefocus)
{
	UpdateSettingFromString(vort, isDefocus, 0.f, 1.f, 0.1f, [](const String &vort) {
		return vort.ToNumber<float>();
	}, [this](float vorticity) {
		VorticityCoeffToTextBox(vorticity);
	}, [this](float vorticity, bool isValid) {
		if (isValid)
		{
			c->SetVorticityCoeff(vorticity);
		}
	});
}

void OptionsView::NotifySettingsChanged(OptionsModel * sender)
{
	temperatureScale->SetOption(sender->GetTemperatureScale()); // has to happen before AmbientAirTempToTextBox is called
	heatSimulation->SetChecked(sender->GetHeatSimulation());
	ambientHeatSimulation->SetChecked(sender->GetAmbientHeatSimulation());
	newtonianGravity->SetChecked(sender->GetNewtonianGravity());
	waterEqualisation->SetChecked(sender->GetWaterEqualisation());
	airMode->SetOption(sender->GetAirMode());
	// Initialize air temp and preview only when the options menu is opened, and not when user is actively editing the textbox
	if (!ambientAirTemp->IsFocused())
	{
		float airTemp = sender->GetAmbientAirTemperature();
		UpdateAmbientAirTempPreview(airTemp, true);
		AmbientAirTempToTextBox(airTemp);
	}
	if (!edgePressure->IsFocused())
	{
		float pres = sender->GetEdgePressure();
		UpdateEdgePressurePreview(pres, true);
		EdgePressureToTextBox(pres);
	}
	if (!fpsLimitText->IsFocused())
	{
		FpsLimitToInterface(sender->GetFpsLimit());
	}
	if (!drawLimitText->IsFocused())
	{
		DrawLimitToInterface(sender->GetDrawLimit());
	}
	// Same for vorticity
	if (!vorticityCoeff->IsFocused())
	{
		VorticityCoeffToTextBox(sender->GetVorticityCoeff());
	}
	convectionMode->SetOption(sender->GetConvectionMode());
	edgeVelocityX = sender->GetEdgeVelocityX();
	edgeVelocityY = sender->GetEdgeVelocityY();
	gravityMode->SetOption(sender->GetGravityMode());
	customGravityX = sender->GetCustomGravityX();
	customGravityY = sender->GetCustomGravityY();
	decoSpace->SetOption(sender->GetDecoSpace());
	edgeMode->SetOption(sender->GetEdgeMode());
	if (scale)
	{
		scale->SetOption(sender->GetScale());
	}
	if (resizable)
	{
		resizable->SetChecked(sender->GetResizable());
	}
	if (fullscreen)
	{
		fullscreen->SetChecked(sender->GetFullscreen());
	}
	if (changeResolution)
	{
		changeResolution->SetChecked(sender->GetChangeResolution());
	}
	if (forceIntegerScaling)
	{
		forceIntegerScaling->SetChecked(sender->GetForceIntegerScaling());
	}
	if (blurryScaling)
	{
		blurryScaling->SetChecked(sender->GetBlurryScaling());
	}
	if (fastquit)
	{
		fastquit->SetChecked(sender->GetFastQuit());
	}
	if (globalQuit)
	{
		globalQuit->SetChecked(sender->GetGlobalQuit());
	}
	if (nativeClipoard)
	{
		nativeClipoard->SetChecked(sender->GetNativeClipoard());
	}
	showAvatars->SetChecked(sender->GetShowAvatars());
	mouseClickRequired->SetChecked(sender->GetMouseClickRequired());
	includePressure->SetChecked(sender->GetIncludePressure());
	perfectCircle->SetChecked(sender->GetPerfectCircle());
	graveExitsConsole->SetChecked(sender->GetGraveExitsConsole());
	threadedRendering->SetChecked(sender->GetThreadedRendering());
	momentumScroll->SetChecked(sender->GetMomentumScroll());
	redirectStd->SetChecked(sender->GetRedirectStd());
	autoStartupRequest->SetChecked(sender->GetAutoStartupRequest());
}

void OptionsView::AttachController(OptionsController * c_)
{
	c = c_;
}

void OptionsView::OnTick()
{
	UpdateStartupRequestStatus();
}

void OptionsView::OnDraw()
{
	Graphics * g = GetGraphics();
	g->DrawFilledRect(RectSized(Position - Vec2{ 1, 1 }, Size + Vec2{ 2, 2 }), 0x000000_rgb);
	g->DrawRect(RectSized(Position, Size), 0xFFFFFF_rgb);
}

void OptionsView::OnTryExit(ExitMethod method)
{
	c->Exit();
}
