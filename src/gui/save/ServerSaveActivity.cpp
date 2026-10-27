#include "ServerSaveActivity.h"
#include "graphics/Graphics.h"
#include "graphics/VideoBuffer.h"
#include "gui/interface/Label.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/Button.h"
#include "gui/interface/Checkbox.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/dialogues/SaveIDMessage.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/InformationMessage.h"
#include "client/Client.h"
#include "client/ThumbnailRendererTask.h"
#include "client/GameSave.h"
#include "client/http/UploadSaveRequest.h"
#include "tasks/Task.h"
#include "gui/Style.h"

class SaveUploadTask: public Task
{
	SaveInfo &save;

	void before() override
	{

	}

	void after() override
	{

	}

	bool doWork() override
	{
		notifyProgress(-1);
		auto uploadSaveRequest = std::make_unique<http::UploadSaveRequest>(save);
		uploadSaveRequest->Start();
		uploadSaveRequest->Wait();
		try
		{
			save.SetID(uploadSaveRequest->Finish());
		}
		catch (const http::RequestError &ex)
		{
			notifyError(ByteString(ex.what()).FromUtf8());
			return false;
		}
		return true;
	}

public:
	SaveUploadTask(SaveInfo &newSave):
		save(newSave)
	{

	}
};

ServerSaveActivity::ServerSaveActivity(std::unique_ptr<SaveInfo> newSave, OnUploaded onUploaded_) :
	WindowActivity(ui::Point(-1, -1), ui::Point(440, 200)),
	thumbnailRenderer(nullptr),
	save(std::move(newSave)),
	onUploaded(onUploaded_),
	saveUploadTask(nullptr)
{
	titleLabel = new ui::Label(ui::Point(4, 5), ui::Point((Size.X/2)-8, 16), "");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);
	CheckName(save->GetName()); //set titleLabel text

	ui::Label * previewLabel = new ui::Label(ui::Point((Size.X/2)+4, 5), ui::Point((Size.X/2)-8, 16), "Предпросмотр:");
	previewLabel->SetTextColour(style::Colour::InformationTitle);
	previewLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	previewLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(previewLabel);

	nameField = new ui::Textbox(ui::Point(8, 25), ui::Point((Size.X/2)-16, 16), save->GetName(), "[имя сохранения]");
	nameField->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	nameField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	nameField->SetActionCallback({ [this] { CheckName(nameField->GetText()); } });
	nameField->SetLimit(50);
	AddComponent(nameField);
	FocusComponent(nameField);

	descriptionField = new ui::Textbox(ui::Point(8, 65), ui::Point((Size.X/2)-16, Size.Y-(65+16+4)), save->GetDescription(), "[описание сохранения]");
	descriptionField->SetMultiline(true);
	descriptionField->SetLimit(254);
	descriptionField->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	descriptionField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(descriptionField);

	publishedCheckbox = new ui::Checkbox(ui::Point(8, 45), ui::Point((Size.X/2)-80, 16), "Опубликовать", "");
	auto user = Client::Ref().GetAuthUser();
	if (!(user && user->Username == save->GetUserName()))
	{
		//Save is not owned by the user, disable by default
		publishedCheckbox->SetChecked(false);
	}
	else
	{
		//Save belongs to the current user, use published state already set
		publishedCheckbox->SetChecked(save->GetPublished());
	}
	AddComponent(publishedCheckbox);

	pausedCheckbox = new ui::Checkbox(ui::Point(160, 45), ui::Point(55, 16), "Пауза", "");
	pausedCheckbox->SetChecked(save->GetGameSave()->paused);
	AddComponent(pausedCheckbox);

	ui::Button * cancelButton = new ui::Button(ui::Point(0, Size.Y-16), ui::Point((Size.X/2)-75, 16), "Отмена");
	cancelButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	cancelButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	cancelButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	cancelButton->SetActionCallback({ [this] {
		Exit();
	} });
	AddComponent(cancelButton);
	SetCancelButton(cancelButton);

	ui::Button * okayButton = new ui::Button(ui::Point((Size.X/2)-76, Size.Y-16), ui::Point(76, 16), "Сохранить");
	okayButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	okayButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	okayButton->Appearance.TextInactive = style::Colour::InformationTitle;
	okayButton->SetActionCallback({ [this] {
		Save();
	} });
	AddComponent(okayButton);
	SetOkayButton(okayButton);

	ui::Button * PublishingInfoButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-42), ui::Point(150, 16), "Информация о публикации");
	PublishingInfoButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	PublishingInfoButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	PublishingInfoButton->Appearance.TextInactive = style::Colour::InformationTitle;
	PublishingInfoButton->SetActionCallback({ [this] {
		ShowPublishingInfo();
	} });
	AddComponent(PublishingInfoButton);

	ui::Button * RulesButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-22), ui::Point(150, 16), "Правила загрузки сохранений");
	RulesButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	RulesButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	RulesButton->Appearance.TextInactive = style::Colour::InformationTitle;
	RulesButton->SetActionCallback({ [this] {
		ShowRules();
	} });
	AddComponent(RulesButton);

	if (save->GetGameSave())
	{
		thumbnailRenderer = new ThumbnailRendererTask(*save->GetGameSave(), Size / 2 - Vec2(16, 16), RendererSettings::decorationAntiClickbait, true);
		thumbnailRenderer->Start();
	}
}

ServerSaveActivity::ServerSaveActivity(std::unique_ptr<SaveInfo> newSave, bool saveNow, OnUploaded onUploaded_) :
	WindowActivity(ui::Point(-1, -1), ui::Point(200, 50)),
	thumbnailRenderer(nullptr),
	save(std::move(newSave)),
	onUploaded(onUploaded_),
	saveUploadTask(nullptr)
{
	ui::Label * titleLabel = new ui::Label(ui::Point(0, 0), Size, "Сохранение на сервер...");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);

	AddAuthorInfo();

	saveUploadTask = new SaveUploadTask(*this->save);
	saveUploadTask->AddTaskListener(this);
	saveUploadTask->Start();
}

void ServerSaveActivity::NotifyDone(Task * task)
{
	if(!task->GetSuccess())
	{
		Exit();
		new ErrorMessage("Ошибка", task->GetError());
	}
	else
	{
		if (onUploaded)
		{
			onUploaded(std::move(save));
		}
		Exit();
	}
}

void ServerSaveActivity::Save()
{
	if (!nameField->GetText().length())
	{
		new ErrorMessage("Ошибка", "Необходимо указать имя сохранения.");
		return;
	}
	auto user = Client::Ref().GetAuthUser();
	if (!(user && user->Username == save->GetUserName()) && publishedCheckbox->GetChecked())
	{
		new ConfirmPrompt("Опубликовать", "Это сохранение создал " + save->GetUserName().FromUtf8() + ", и вы собираетесь опубликовать его под своим именем; если автор не дал вам на это разрешение, снимите галочку «Опубликовать», в противном случае продолжайте", { [this] {
			saveUpload();
		} });
	}
	else
	{
		saveUpload();
	}
}

void ServerSaveActivity::AddAuthorInfo()
{
	Json::Value serverSaveInfo;
	serverSaveInfo["type"] = "save";
	serverSaveInfo["id"] = save->GetID();
	auto user = Client::Ref().GetAuthUser();
	serverSaveInfo["username"] = user ? user->Username : ByteString("");
	serverSaveInfo["title"] = save->GetName().ToUtf8();
	serverSaveInfo["description"] = save->GetDescription().ToUtf8();
	serverSaveInfo["published"] = (int)save->GetPublished();
	serverSaveInfo["date"] = (Json::Value::UInt64)time(nullptr);
	Client::Ref().SaveAuthorInfo(&serverSaveInfo);
	{
		auto gameSave = save->TakeGameSave();
		gameSave->authors = serverSaveInfo;
		save->SetGameSave(std::move(gameSave));
	}
}

void ServerSaveActivity::saveUpload()
{
	okayButton->Enabled = false;
	save->SetName(nameField->GetText());
	save->SetDescription(descriptionField->GetText());
	save->SetPublished(publishedCheckbox->GetChecked());
	auto user = Client::Ref().GetAuthUser();
	save->SetUserName(user ? user->Username : ByteString(""));
	save->SetID(0);
	{
		auto gameSave = save->TakeGameSave();
		gameSave->paused = pausedCheckbox->GetChecked();
		save->SetGameSave(std::move(gameSave));
	}
	AddAuthorInfo();
	uploadSaveRequest = std::make_unique<http::UploadSaveRequest>(*save);
	uploadSaveRequest->Start();
}

void ServerSaveActivity::Exit()
{
	WindowActivity::Exit();
}

void ServerSaveActivity::ShowPublishingInfo()
{
	String info =
		"В The Powder Toy вы можете сохранять симуляции в свой аккаунт на двух уровнях приватности: опубликованные и неопубликованные. Выбирается это галочкой «Опубликовать». По умолчанию сохранения не опубликованы, поэтому, если вы не поставите эту галочку, никто не сможет увидеть ваши сохранения.\n"
		"\n"
		"\btОпубликованные сохранения\bw попадают в ленту «По дате» и будут видны многим людям. Такие сохранения также влияют на ваш Средний балл, который публично отображается на странице вашего профиля на сайте. Публикуйте сохранения, которые хотите показать людям, чтобы они могли их комментировать и голосовать.\n"
		"\btНеопубликованные сохранения\bw не показываются в ленте «По дате» и не влияют на Средний балл. Однако они не полностью приватны: любой, кто знает ID сохранения, сможет его открыть. Вы можете раздавать ID сохранения, чтобы показать его конкретным людям, но не позволять всем подряд его увидеть.\n"
		"\n"
		"Чтобы быстро пересохранить сохранение, откройте его и нажмите левую часть кнопки пересохранения: \bt«Перезалить текущую симуляцию»\bw. Если нужно изменить описание или статус публикации, нажмите правую часть: \bt«Изменить свойства симуляции»\bw. Учтите, что имя сохранения изменить нельзя: это создаст совершенно новое сохранение без комментариев, голосов и тегов, отдельное от исходного.\n"
		"Возможно, вы захотите опубликовать готовое неопубликованное сохранение или снять публикацию с некоторых опубликованных. Для этого откройте сохранение, выберите кнопку «Изменить свойства симуляции» и поменяйте статус публикации. Также вы можете \btснять публикацию или удалить сохранения\bw, выбрав их в разделе «мои» браузера и нажав одну из появившихся внизу кнопок.\n"
		"Если сохранению меньше недели и оно быстро набирает популярность, оно автоматически попадает на \btглавную страницу\bw. Туда попадают только опубликованные сохранения. Модераторы также могут продвинуть любое сохранение на главную, но это происходит редко. Они могут и убрать с главной любое сохранение, нарушающее правила или, по их мнению, туда не подходящее.\n"
		"Создав сохранение, вы можете пересохранять его сколько угодно раз. Хранится небольшая \btистория сохранений\bw: нажмите правой кнопкой на любое сохранение в браузере сохранений и выберите «Просмотреть историю». Это полезно, если вы случайно сохранили что-то не то и хотите вернуться к старой версии.\n"
		;

	new InformationMessage("Информация о публикации", info, true);
}

void ServerSaveActivity::ShowRules()
{
	String rules =
		"\boРаздел S: Социальные правила и правила сообщества\n"
		"\bwВо время общения в сообществе соблюдайте несколько правил. Эти правила обеспечиваются сотрудниками, а о нарушениях пользователи могут сообщить нам. Раздел касается загружаемых сохранений, комментариев, форумов и других областей сообщества.\n"
		"\n"
		"\bt1. Старайтесь использовать правильную грамматику.\bw Английский — официальный язык сообщества, но его использование необязательно в региональных или культурных группах. Если вы плохо пишете по-английски, советуем пользоваться Google Translate.\n"
		"\bt2. Не спамите.\bw Единого определения здесь нет, но обычно всё очевидно. Кроме того, перечисленное ниже считается спамом и может быть скрыто или удалено:\n"
		   "- Публикация нескольких тем на одну тему. Старайтесь объединять темы с отзывами или предложениями по игре в одну тему.\n"
		   "- Поднятие старых тем ответами. Это называется «некропостинг». Содержимое темы может устареть (исправления проблем, идеи и т.д.). Рекомендуем создавать новую тему для актуального ответа.\n"
		   "- Ответы вида «+1» или другие короткие сообщения. Нет нужды постоянно поднимать тему и затруднять поиск ответов. Ответы хороши для конструктивной обратной связи, а кнопка «+1» — чтобы показать поддержку контента.\n"
		   "- Чрезмерно длинные или бессмысленные комментарии. Комментарии, где повторяется одна буква или нет цели, подпадают под это правило. Комментарии на другом языке освобождаются от него.\n"
		   "- Чрезмерное форматирование. ЗАГЛАВНЫЕ, полужирный и курсив хороши в меру, но не используйте их во всём сообщении.\n"
		"\bt3. Сведите ругательства к минимуму.\bw Комментарии или сохранения с ругательствами могут быть удалены. Это касается и ругательств на других языках.\n"
		"\bt4. Не загружайте материалы сексуального характера, оскорбительные или другие неподобающие материалы.\bw\n"
		   "- Сюда входят, но не ограничиваются: секс, наркотики, расизм, чрезмерная политика или всё, что оскорбляет группу людей.\n"
		   "- Упоминание этих тем на других языках также запрещено. Не пытайтесь обойти это правило.\n"
		   "- Запрещено размещение ссылок или изображений, нарушающих это правило. Включая ссылки или текст в информации профиля.\n"
		"\bt5. Не рекламируйте сторонние игры, сайты и другие места, не связанные с The Powder Toy.\bw\n"
		   "- В основном правило направлено на то, чтобы люди не рекламировали свои игры и продукты.\n"
		   "- Несанкционированные или неофициальные места общения сообщества, такие как Discord, запрещены.\n"
		"\bt6. Троллинг запрещён.\bw Как и в некоторых правилах, чёткого определения нет. Пользователи, которые постоянно троллят, с гораздо большей вероятностью получат бан, причём более длительный.\n"
		"\bt7. Не выдавайте себя за других.\bw Запрещена регистрация аккаунтов с именами, намеренно похожими на имена других пользователей нашего или иных онлайн-сообществ.\n"
		"\bt8. Не обсуждайте решения и действия модераторов.\bw Если есть проблема с баном вашего аккаунта или удалением контента, свяжитесь с модератором через систему сообщений. В остальных случаях обсуждения действий модераторов следует избегать.\n"
		"\bt9. Избегайте «модераторства с заднего сиденья».\bw Решения принимают модераторы. Пользователи не должны угрожать банами или перечислять возможные последствия нарушения правил. При возможной проблеме или неуверенности рекомендуем сообщить о ней через кнопку «Пожаловаться» или через систему сообщений на сайте.\n"
		"\bt10. Запрещено оправдывать нарушение общепринятых законов.\bw Применяется ли закон какой-то конкретной страны, неясно, но есть общие законы, которые стоит знать. Сюда входят, но не ограничиваются:\n"
		   "- Пиратство ПО, музыки, бубликов и т.п.\n"
		   "- Взлом / кража аккаунтов\n"
		   "- Кража / мошенничество\n"
		"\bt11. Не преследуйте и не притесняйте пользователей.\bw В последние годы это растущая проблема, осуществляемая разными способами, но обычно сюда входят:\n"
		   "- «Доксинг» пользователей с целью выяснить, где они живут или кто они на самом деле\n"
		   "- Постоянные сообщения пользователю, который не хочет контакта\n"
		   "- Массовые минусы сохранениям\n"
		   "- Грубые или неуместные комментарии к чужому контенту (сохранения, темы форума и т.д.)\n"
		   "- Принуждение группы пользователей к «атаке» на пользователя\n"
		   "- Личные ссоры или ненависть. Это может быть спор в комментариях или создание «ненавистных» сохранений\n"
		   "- Дискриминация людей вообще: религиозная, этническая и т.д.\n"
		"\n"
		"\boРаздел G: Правила игры\n"
		"\bwЭтот раздел правил касается действий в игре. Раздел S также действует в игре, но эти правила более специфичны для игрового взаимодействия.\n"
		"\bt1. Не присваивайте чужой труд.\bw Это может быть простое повторное размещение чужого сохранения или использование больших частей сохранений. Производные работы разрешены при правильном использовании. При использовании чужой работы вы обязаны указать автора, если он не указал иных условий. Производная работа определяется инновационностью и процентом оригинальности (насколько она оригинальна по сравнению с чужой). Украденные сохранения будут сняты с публикации или отключены.\n"
		"\bt2. Само-голосование и мошенничество с голосами запрещены.\bw Это создание нескольких аккаунтов для голосования за свои или чужие сохранения. Мы строго следим за этим, поэтому учтите: успешных апелляций по банам очень мало. Убедитесь, что вы и другие аккаунты не голосуете из одного дома. Все альтернативные аккаунты будут забанены навсегда, основной аккаунт — временно, а затронутые сохранения будут отключены.\n"
		"\bt3. Просьбы о голосах любого рода не одобряются.\bw Сохранения, делающие это, будут сняты с публикации до исправления. Примеры, подпадающие под это правило:\n"
		   "- Знаки, намекающие на голосование. Под это правило подпадает фирменная зелёная стрелка или призыв голосовать.\n"
		   "- Приёмы с просьбой голосовать. Например, обмен количества голосов на что-то вроде «100 голосов — и я сделаю лучшую версию». Это называется фарм голосов и запрещено.\n"
		   "- Просьба голосовать в обмен на использование сохранения или по любой другой причине запрещена.\n"
		"\bt4. Не спамите.\bw Как уже говорилось, единого определения спама нет. Примеры того, что может считаться спамом:\n"
		   "- Загрузка или перезагрузка похожих сохранений за короткое время. Не пытайтесь обходить систему, чтобы ваши сохранения увидели/оценили. Сюда входит загрузка «мусорных» или «пустых» сохранений. Такие сохранения будут сняты с публикации.\n"
		   "- Загрузка текстовых сохранений. Это могут быть объявления или просьбы о помощи. Для многих таких целей у нас есть форумы и комментарии. Такие сохранения будут убраны с главной страницы.\n"
		   "- Художественные сохранения строго не запрещены, но могут быть убраны с главной. Нам нравится творческое использование разнообразных элементов. Отсутствие этих факторов (например, только декорации) обычно приводит к удалению с главной\n"
		"\bt5. Не загружайте материалы сексуального характера или другие неподобающие материалы. Такие сохранения будут удалены, а вы получите бан.\bw\n"
		   "- Сюда входят, но не ограничиваются: секс, наркотики, расизм, чрезмерная политика или всё, что оскорбляет группу людей.\n"
		   "- Не пытайтесь обойти правило. Всё, что прямо или косвенно относится к этим понятиям/идеям, подпадает под него.\n"
		   "- Упоминание этих тем на других языках также запрещено. Не пытайтесь обойти это правило.\n"
		   "- Запрещено размещение ссылок или изображений, нарушающих это правило. Включая ссылки или текст в информации профиля.\n"
		"\bt6. Построение изображений (image plotting) строго запрещено.\bw Это использование скриптов или сторонних инструментов для построения/создания сохранения за вас. Сохранения с CGI будут удалены, вы можете получить бан.\n"
		"\bt7. Сведите количество логотипов и знаков к минимуму.\bw Такие сохранения могут быть убраны с главной страницы. Под ограничение подпадают:\n"
		   "- Чрезмерное количество размещённых логотипов\n"
		   "- Знаки без цели\n"
		   "- Фальшивые знаки обновлений или уведомлений\n"
		   "- Ссылки на другие сохранения без связи с темой\n"
		"\bt8. Не ставьте оффтопик или неподобающие теги.\bw Теги нужны только для улучшения поиска. Обычно это одно слово-описание сохранения. Предложения или субъективные теги могут быть удалены. За неподобающие или оскорбительные теги, скорее всего, будет бан.\n"
		"\bt9. Запрещены сохранения, намеренно вызывающие лаги или вылеты.\bw Если большинство пользователей пишут, что сохранение вызывает вылеты или лаги, оно подпадает под это правило. Такие сохранения будут убраны с главной или отключены.\n"
		"\bt10. Не злоупотребляйте системой жалоб.\bw Причины вроде «плохое сохранение» или бессмыслица тратят наше время. Если проблема не касается возможного нарушения правил или вопросов сообщества, воздержитесь от жалобы. Если вы считаете, что сохранение нарушает правила или создаёт проблему для сообщества, жалобу всё равно отправляйте! За добросовестную жалобу бан никогда не наступит.\n"
		"\bt11. Не просите убрать сохранения с главной страницы.\bw Если сохранение не нарушает правила, оно останется на главной. Для художественных сохранений исключений нет, на них тоже не нужно жаловаться.\n"
		"\n"
		"\boРаздел R: Прочее\n"
		"\bwМодераторы могут трактовать эти правила по своему усмотрению. Не все правила равны: некоторые соблюдаются строже других. Окончательное решение о том, что против правил, а что нет, принимают модераторы, но мы постарались охватить здесь всё нежелательное поведение. При обновлении правил в этой теме будет размещено уведомление.\n"
		"\n"
		"Нарушение этих правил может привести к удалению публикаций/комментариев, снятию публикации или отключению сохранений, удалению с главной страницы, а в более крайних случаях — к временному или постоянному бану. Действуют различные ручные и автоматические меры по соблюдению правил. Строгость и итоговые решения могут различаться у разных модераторов.\n"
		"\n"
		"Если у вас есть вопросы о том, что против правил, а что нет, смело свяжитесь с модератором.";

	new InformationMessage("Правила загрузки сохранений", rules, true);
}

void ServerSaveActivity::CheckName(String newname)
{
	auto user = Client::Ref().GetAuthUser();
	if (newname.length() && newname == save->GetName() && user && save->GetUserName() == user->Username)
		titleLabel->SetText("Изменить свойства симуляции:");
	else
		titleLabel->SetText("Загрузить новую симуляцию:");
}

void ServerSaveActivity::OnTick()
{
	if (thumbnailRenderer)
	{
		thumbnailRenderer->Poll();
		if (thumbnailRenderer->GetDone())
		{
			thumbnail = thumbnailRenderer->Finish();
			thumbnailRenderer = nullptr;
		}
	}

	if (uploadSaveRequest && uploadSaveRequest->CheckDone())
	{
		okayButton->Enabled = true;
		try
		{
			save->SetID(uploadSaveRequest->Finish());
			Exit();
			new SaveIDMessage(save->GetID());
			if (onUploaded)
			{
				onUploaded(std::move(save));
			}
		}
		catch (const http::RequestError &ex)
		{
			new ErrorMessage("Ошибка", "Не удалось загрузить. Ошибка:\n" + ByteString(ex.what()).FromUtf8());
		}
		uploadSaveRequest.reset();
	}

	if(saveUploadTask)
		saveUploadTask->Poll();
}

void ServerSaveActivity::OnDraw()
{
	Graphics * g = GetGraphics();
	g->BlendRGBAImage(saveToServerImage->data(), RectSized(Vec2(-10, 0), saveToServerImage->Size()));
	g->DrawFilledRect(RectSized(Position, Size).Inset(-1), 0x000000_rgb);
	g->DrawRect(RectSized(Position, Size), 0xFFFFFF_rgb);

	if (Size.X > 220)
		g->DrawLine(Position + Vec2(Size.X / 2 - 1, 0), Position + Vec2(Size.X / 2 - 1, Size.Y - 1), 0xFFFFFF_rgb);

	if (thumbnail)
	{
		auto rect = RectSized(Position + Vec2(Size.X / 2 + (Size.X / 2 - thumbnail->Size().X) / 2, 25), thumbnail->Size());
		g->BlendImage(thumbnail->Data(), 0xFF, rect);
		g->DrawRect(rect, 0xB4B4B4_rgb);
	}
}

ServerSaveActivity::~ServerSaveActivity()
{
	if (thumbnailRenderer)
	{
		thumbnailRenderer->Abandon();
	}
	delete saveUploadTask;
}
