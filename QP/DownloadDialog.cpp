#include "DownloadDialog.h"
#include "utils.h"
#include "Settings.h"
#include "Network.h"
#include <QtWidgets>
#include <tuple>

enum { TitleColum, DurationColum, TagColum };
class ContentTreeItem :public QTreeWidgetItem
{
public:
	static  constexpr  int SectionItemType = QTreeWidgetItem::UserType;
	static  constexpr  int ContentItemType = QTreeWidgetItem::UserType + 1;
	static  constexpr  int ItemRole = Qt::UserRole + 1;
private:
	ContentTreeItem(const Extractor::ContentItem& item) :QTreeWidgetItem(ContentItemType)
	{
		setText(TitleColum, item.title);
		setToolTip(TitleColum, item.title);
		QTreeWidgetItem::setData(0, ItemRole, item.id);
		QTreeWidgetItem::setData(0, Qt::CheckStateRole, Qt::Unchecked);
		if (item.flags & ContentItemFlag::Disabled)
		{
			setDisabled(true);
			setFlags(Qt::ItemNeverHasChildren);
		}
		else
		{
			setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren);
		}
		QString tag;
		if (item.flags & ContentItemFlag::VipOnly)
		{
			tag = QStringLiteral("会员");
		}
		else if (item.flags & ContentItemFlag::PayOnly)
		{
			tag = QStringLiteral("付费");
		}
		else if (item.flags & ContentItemFlag::AllowWaitFree)
		{
			tag = QStringLiteral("等免");
		}
		if (!tag.isEmpty())
		{
			setText(TagColum, tag);
			setForeground(TagColum, Qt::white);
			setBackground(TagColum, B23Style::Pink);
		}

		if (item.durationInSec > 0)
		{
			setText(DurationColum, utils::secs2HmsStr(item.durationInSec));
			setToolTip(DurationColum, utils::secs2HmsStrLocaleStr(item.durationInSec));
		}
	}
	ContentTreeItem(const Extractor::Section& section) :QTreeWidgetItem(SectionItemType)
	{
		setText(TitleColum, section.title);
		setChildIndicatorPolicy(ChildIndicatorPolicy::ShowIndicator);
		auto flags = Qt::ItemIsEnabled | Qt::ItemIsAutoTristate;
		if (section.episodes.size() != 0)
		{
			flags |= Qt::ItemIsUserCheckable;
			QTreeWidgetItem::setData(0, Qt::CheckStateRole, Qt::Unchecked);
		}
		setFlags(flags);

		bool allchildrenDisabled = true;
		for (auto& ep : section.episodes)
		{
			if (!(ep.flags & ContentItemFlag::Disabled))
			{
				allchildrenDisabled = false;
				break;
			}
		}
		if (allchildrenDisabled)
		{
			setDisabled(true);
		}
	}
public:
	QnList qnList;
	static QList<QTreeWidgetItem*> createItems(const QList<Extractor::ContentItem>& items)
	{
		QList<QTreeWidgetItem*> treeItems;
		treeItems.reserve(items.size());
		for (auto& item : items)
		{
			treeItems.append(new ContentTreeItem(item));
		}
		return treeItems;
	}
	static QList<QTreeWidgetItem*> createSectionItems(const QList<Extractor::Section>& sections)
	{
		QList<QTreeWidgetItem*> treeItems;
		treeItems.reserve(sections.size());
		for (auto& section : sections)
		{
			auto item = new ContentTreeItem(section);
			item->addChildren(createItems(section.episodes));
			treeItems.append(item);
		}
		return treeItems;

	}

	qint64 contentItemId()
	{
		return data(0, ItemRole).toLongLong();
	}
	void setChecked(bool checked)
	{
		qDebug() << __func__;
		QTreeWidgetItem::setData(0, Qt::CheckStateRole, checked ? Qt::Checked : Qt::Unchecked);
	}
	ContentTreeWidget* treeWidget()
	{
		return static_cast<ContentTreeWidget*>(QTreeWidgetItem::treeWidget());
	}
	QString longTitle()
	{
		// 如果 section/item 标题为"正片", 则视其为空
	   // 比如电影《星际穿越》, 它的标题为"星际穿越", 有两个section("正片"和"预告花絮"),
	   //     标题为"正片"的section包含一个标题为"正片"的视频
		auto clearIFZP = [](QString& s)
		{
			if (s == QStringLiteral("正片"))
			{
				s.clear();
			}
		};
		QString ret;
		if (parent() != nullptr)
		{
			auto sectionTitle = parent()->text(0);
			auto itemtitle = text(0);
			clearIFZP(sectionTitle);
			clearIFZP(itemtitle);
			if (sectionTitle.isEmpty())
			{
				ret = itemtitle;
			}
			else if (!itemtitle.isEmpty())
			{
				ret = sectionTitle + " " + itemtitle;
			}
		}
		else
		{
			ret = text(0);
			clearIFZP(ret);
		}
		return utils::legalizedFileName(ret);
	}
private:
	void setData(int column, int role, const QVariant& value) override
	{
		if (role == Qt::CheckStateRole)
		{
			if (type() == SectionItemType)
			{
				treeWidget()->beginSelMultItems();
				QTreeWidgetItem::setData(column, role, value);
				treeWidget()->endSelMultItems();
				return;
			}
			else if (Type() == ContentItemType)
			{
				if (!isDisabled())
				{
					setSelected(value == Qt::Checked);
				}
				return;
			}
		}
		QTreeWidgetItem::setData(column, role, value);
	}
};
const int ContentTreeWidget::RowSpacing = 2;
ContentTreeWidget::ContentTreeWidget(const Extractor::Result* content, QWidget* parent) :QTreeWidget(parent)
{
	Q_ASSERT(content->type != ContentType::LIVE);

	setAllColumnsShowFocus(true);
	setDragDropMode(DragDropMode::NoDragDrop);
	setSelectionMode(QAbstractItemView::MultiSelection);
	setStyleSheet(QStringLiteral("QTreeView::item{margin-top:%1px;margin-bottom:%1px;}").arg(RowSpacing / 2));
	setMidLineWidth(fontMetrics().horizontalAdvance("+第001话 不可思议的魔法书+++1234:00:00 +++会员+"));

	setColumnCount(3);
	if (content->type == ContentType::COMIC)
	{
		setHeaderLabels({ "标题","","" });
	}
	else
	{
		setHeaderLabels({ "标题","时长","" });
	}
	header()->setDefaultAlignment(Qt::AlignCenter);
	header()->setSectionsMovable(false);
	header()->setStretchLastSection(false);
	header()->setSectionResizeMode(TitleColum, QHeaderView::Stretch);
	header()->setSectionResizeMode(DurationColum, QHeaderView::Fixed);
	header()->setSectionResizeMode(TagColum, QHeaderView::ResizeToContents);
	header()->resizeSection(1, fontMetrics().horizontalAdvance("1234:12:12"));

	auto type = content->type;
	switch (type)
	{
	case ContentType::PGC:
	{
		auto sectionListRes = static_cast<const Extractor::SectionListResult*>(content);
		if (sectionListRes->setions.size() == 1)
		{
			auto videos = sectionListRes->setions.first().episodes;
			addTopLevelItems(ContentTreeItem::createItems(videos));
		}
		else
		{
			auto sections = sectionListRes->setions;
			addTopLevelItems(ContentTreeItem::createSectionItems(sections));
		}
	}
	break;
	case ContentType::UGC:
	case ContentType::PUGV:
	case ContentType::COMIC:
	{
		auto itemListRes = static_cast<const Extractor::ItemListResult*>(content);
		addTopLevelItems(ContentTreeItem::createItems(itemListRes->items));
		break;
	}
	default:
		break;
	}

	for (auto it = QTreeWidgetItemIterator(this); (*it); it++)
	{
		auto item = (*it);
		if (!item->isDisabled() && item->type() == ContentTreeItem::ContentItemType)
		{
			enabledItemCnt++;
		}
	}
	// Extractor 返回的内容包含了所有 分P/分集，
   //   如果用户输入的链接是指向单个视频的类型，那么默认(只)选中这个视频并set为currentItem
   //   否则，选择第一个视频作为currentItem
   //   在创建tree结束后，DownloadDialog会根据currentItem获取画质列表
	for (auto it = QTreeWidgetItemIterator(this); *it; ++it)
	{
		auto item = static_cast<ContentTreeItem*>(*it);
		if (!item->isDisabled() && item->type() == ContentTreeItem::ContentItemType)
		{
			if (content->focusItemId == 0)
			{
				setCurrentItem(item, 0, QItemSelectionModel::NoUpdate);
				break;
			}
			else if (item->contentItemId() == content->focusItemId)
			{
				setCurrentItem(item);
				break;
			}
		}
	}
	expandAll();
	if (QTreeWidget::currentItem() != nullptr)
	{
		scrollToItem(QTreeWidget::currentItem());
	}
	QTimer::singleShot(0, this, [this] {setFocus(); });
}

ContentTreeItem* ContentTreeWidget::currentItem() const
{
	return dynamic_cast<ContentTreeItem*>(QTreeWidget::currentItem());
}

void ContentTreeWidget::selectAll()
{
	/* QAbstractItemView::selectAll() only select top level items;
   * QTreeView::selectAll() doesn't select items whose parent is not expanded;
   * QTreeWidget doesn't override selectAll()
   */
	beginSelMultItems();
	auto it = QTreeWidgetItemIterator(this);
	for (; *it; ++it)
	{
		auto item = *it;
		if (item->type() == ContentTreeItem::ContentItemType && !item->isDisabled())
		{
			item->setSelected(true);
		}
	}
	endSelMultItems();
}

void ContentTreeWidget::beginSelMultItems()
{
	blockSigSelCntChanged = true;
	selCntBeforeBlock = selCnt;
}

void ContentTreeWidget::endSelMultItems()
{
	blockSigSelCntChanged = false;
	emit selectedItemCntChanged(selCnt, selCntBeforeBlock);
}

QList<QTreeWidgetItem*> ContentTreeWidget::selection2Items(const QItemSelection& selection)
{
	QList<QTreeWidgetItem*> ret;

	for (auto& range : selection)
	{
		auto index = range.topLeft();
		for (int row = range.top(); row <=range.bottom(); row++)
		{
			auto item = itemFromIndex(index.sibling(row, 0));
			if (item->type() == ContentTreeItem::SectionItemType)
			{
				continue;;
			}
			ret.append(item);
		}
	}

	return  ret;
}

void ContentTreeWidget::drawRow(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	auto opt = option;
	opt.rect.adjust(0, RowSpacing / 2, 0, -RowSpacing / 2);
	QTreeWidget::drawRow(painter, opt, index);
}

void ContentTreeWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QTreeWidget::selectionChanged(selected, deselected);

	int prevSelCnt = selCnt;

	auto sel = selection2Items(selected);
	selCnt += sel.size();
	for (auto&& item : sel)
	{
		item->QTreeWidgetItem::setData(0, Qt::CheckStateRole, Qt::Checked);
	}
	auto desel = selection2Items(deselected);
	selCnt -= desel.size();
	for (auto&& item : desel)
	{
		item->QTreeWidgetItem::setData(0, Qt::CheckStateRole, Qt::Unchecked);
	}
	if (!blockSigSelCntChanged)
	{
		emit selectedItemCntChanged(selCnt, prevSelCnt);
	}
}


class ActivityTipDialog :public QDialog
{
public:
	ActivityTipDialog(QString title, QWidget* parent)
	{
		auto layout = new QVBoxLayout(this);
		layout->addWidget(new QLabel(title));
		auto cancelBtn = new QPushButton("Cancel");
		layout->addWidget(cancelBtn);
		layout->setSizeConstraint(QLayout::SetFixedSize);
		connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
	}
};

DownloadDialog::DownloadDialog(const QString& url, QWidget* parent)
{
	setWindowTitle("下载");
	extractor = new Extractor;

	connect(extractor, &Extractor::success, this, &DownloadDialog::setupUi);
	connect(extractor, &Extractor::errorOccured, this, &DownloadDialog::extratorErrorOccured);

	activityTipDialog = new ActivityTipDialog("获取内中中...", parent);
	activityTipDialog->setWindowTitle("Xiazai");
	connect(activityTipDialog, &QDialog::rejected, this, &DownloadDialog::abortExtract);

	QTimer::singleShot(0, this, [this, url] {extractor->start(url); });
}

DownloadDialog::~DownloadDialog()
{
}

QList<AbstractDownloadTask*> DownloadDialog::getDownloadTasks()
{
	Settings::inst()->setValue("lastDir", pathLabel->text());
	QList<AbstractDownloadTask*> tasks;

	auto dir = QDir(pathLabel->text());
	int qn = (qnCombobox == nullptr ? 0 : qnCombobox->currentData().toInt());
	auto title = utils::legalizedFileName(titleLabel->text());

	if (contentType == ContentType::LIVE)
	{
		//tasks.append(new live)
		throw QString("not finished");
	}
	else
	{
		QList<std::tuple<qint64, QString>> metaInfos;
		for (auto item : tree->selectedItems())
		{
			auto videoItem = static_cast<ContentTreeItem*> (item);
			if (videoItem->type() != ContentTreeItem::ContentItemType)
			{
				continue;
			}
			auto itemTitle = videoItem->longTitle();
			metaInfos.emplaceBack(videoItem->contentItemId(), (itemTitle.isEmpty() ? title : title + " " + itemTitle));
		}
		for (auto& [itemId, name] : metaInfos)
		{
			AbstractDownloadTask* task = nullptr;
			auto path = dir.filePath(name);
			switch (contentType)
			{
			case ContentType::PGC:
				task = new PgcDownloadTask(contentItemId, itemId, qn, path);
				break;
			default:
				break;
			}
			if (task != nullptr)
			{
				tasks.append(task);
			}
		}
	}
	return tasks;
}

void DownloadDialog::open()
{
	activityTipDialog->open();
}

void DownloadDialog::abortExtract()
{
	extractor->abort();
	extractor->deleteLater();
	activityTipDialog->deleteLater();
	reject();
}

void DownloadDialog::extratorErrorOccured(const QString& errorString)
{
	extractor->deleteLater();
	activityTipDialog->accept();
	activityTipDialog->deleteLater();
	QMessageBox::critical(this->parentWidget(), "下载", errorString);
	reject();
}
static auto qnComboBoxToolTip =
"点击 \"获取当前项画质\" 按钮来获取单个视频的画质.\n"
"非 * 开头的为该视频可用的画质\n"
"* 开头的画质不属于该视频, 但其他视频可能有\n"
"注: 未登录/无会员 可能导致较高画质不可用";
void DownloadDialog::setupUi()
{
	qDebug() << __func__;
	activityTipDialog->accept();
	activityTipDialog->deleteLater();
	QTimer::singleShot(0, this, [this] {QDialog::open(); });

	auto mainlayout = new QVBoxLayout(this);
	auto result = extractor->getResult();

	extractor->deleteLater();

	contentType = result->type;
	contentId = result->id;

	qDebug() << "type " << contentType << "contentid " << contentId;
	titleLabel = new QLabel(result->title);
	titleLabel->setWordWrap(true);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto font = titleLabel->font();
	font.setPointSize(font.pointSize() + 3);
	titleLabel->setFont(font);
	titleLabel->setMinimumWidth(QFontMetrics(font).horizontalAdvance("++魔卡少女樱 Clear Card篇++"));

	mainlayout->addWidget(titleLabel);
	if (contentType != ContentType::LIVE)
	{
		auto selLayout = new QHBoxLayout;
		selAllBtn = new QToolButton;
		selAllBtn->setText("全选");
		selAllBtn->setAutoRaise(true);
		selAllBtn->setCursor(Qt::PointingHandCursor);

		selContLabel = new QLabel;
		selLayout->addWidget(selAllBtn);
		selLayout->addStretch(1);
		selLayout->addWidget(selContLabel);
		mainlayout->addLayout(selLayout);

		tree = new ContentTreeWidget(result.get());
		tree->setFixedHeight(196);
		mainlayout->addWidget(tree, 1);
	}
	if (contentType == ContentType::UGC && tree->getEnabledItemCount() == 1)
	{
		selAllBtn->hide();
		selContLabel->hide();
		tree->hide();
		tree->currentItem()->setText(0, QString());
	}
	if (contentType != ContentType::COMIC)
	{
		auto qnlayout = new QHBoxLayout;
		qnlayout->addWidget(new QLabel("画质:"));
		qnCombobox = new QComboBox;
		qnCombobox->setFocusPolicy(Qt::NoFocus);
		qnCombobox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		auto qnTipLabel = new QLabel;
		auto infoIcon = style()->standardIcon(QStyle::SP_MessageBoxInformation);
		qnTipLabel->setPixmap(infoIcon.pixmap(16, 16));
		qnTipLabel->setToolTip(qnComboBoxToolTip);
		qnlayout->addWidget(qnCombobox);
		qnlayout->addWidget(qnTipLabel);
		qnlayout->addStretch(1);
		if (tree != nullptr)
		{
			getQnListBtn = new QPushButton("获取当前画质");
			getQnListBtn->setFlat(true);
			getQnListBtn->setCursor(Qt::PointingHandCursor);
			qnlayout->addSpacing(15);
			qnlayout->addWidget(getQnListBtn);
			connect(getQnListBtn, &QPushButton::clicked, this, &DownloadDialog::startGetCurrentItemQnList);
		}
		mainlayout->addLayout(qnlayout);
	}
	auto pathLayoutFrame = new QFrame;
	pathLayoutFrame->setContentsMargins(1, 1, 1, 1);
	pathLayoutFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	pathLayoutFrame->setLineWidth(1);
	pathLayoutFrame->setMidLineWidth(0);

	auto pathLayout = new QHBoxLayout(pathLayoutFrame);
	pathLayout->addWidget(new QLabel("下载到:"));
	pathLayout->setContentsMargins(2, 0, 1, 0);
	pathLabel = new ElidedTextLabel;

	auto lastDir = Settings::inst()->value("lastDir").toString();
	if (lastDir.isEmpty() || !QDir(lastDir).exists())
	{
		pathLabel->setText(QDir::currentPath());
	}
	else
	{
		pathLabel->setToolTip(lastDir);
	}
	pathLabel->setElideMode(Qt::ElideMiddle);
	pathLabel->setHintWidthToString("/someDir/anime/百变小樱 第01话 不可思议的魔法书");

	selPathButton = new QPushButton;
	selPathButton->setToolTip("选择文件夹");
	selPathButton->setIconSize(QSize(20, 20));
	selPathButton->setIcon(QIcon(":/icons/folder.svg"));
	selPathButton->setFlat(true);
	selPathButton->setCursor(Qt::PointingHandCursor);
	selPathButton->setStyleSheet("QPushButton:pressed{border:none}");

	pathLayout->addWidget(pathLabel, 1);
	pathLayout->addWidget(selPathButton);
	mainlayout->addWidget(pathLayoutFrame);

	auto bottomLayout = new QHBoxLayout;
	bottomLayout->addStretch(1);
	okButton = new QPushButton("下载");
	okButton->setEnabled(tree == nullptr ? true : tree->getSelectedItemCount() > 0);
	auto cancelButton = new QPushButton("取消");
	bottomLayout->addWidget(okButton);
	bottomLayout->addWidget(cancelButton);
	mainlayout->addLayout(bottomLayout);

	addPlayDirect(bottomLayout);

	connect(okButton, &QPushButton::clicked, this, &DownloadDialog::accept);
	connect(cancelButton, &QPushButton::clicked, this, &DownloadDialog::reject);
	connect(selPathButton, &QAbstractButton::clicked, this, &DownloadDialog::selectPath);

	if (tree != nullptr)
	{

		connect(selAllBtn, &QAbstractButton::clicked, this, &DownloadDialog::selAllBtnClicked);
		connect(tree, &ContentTreeWidget::selectedItemCntChanged, this, &DownloadDialog::selCntChanged);
		selCntChanged(tree->getSelectedItemCount(), 0);

		connect(tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* cur, QTreeWidgetItem* pre)
			{
				if (getQnListBtn != nullptr)
				{
					getQnListBtn->setDisabled(cur == nullptr || cur->type() != ContentTreeItem::ContentItemType);
				}
				qDebug() << "current changed to" << (cur == nullptr ? "NULL" : cur->text(0))
					<< "(previous:" << (pre == nullptr ? "NULL" : pre->text(0));
			});
	}
	if (qnCombobox != nullptr && (tree == nullptr || tree->currentItem() != nullptr))
	{
		startGetCurrentItemQnList();
	}

}

void DownloadDialog::selAllBtnClicked()
{
	if (tree->getSelectedItemCount() == tree->getEnabledItemCount())
	{
		tree->clearSelection();
	}
	else
	{
		tree->selectAll();
	}
}

void DownloadDialog::selCntChanged(int curr, int prev)
{
	qDebug()<<__func__ <<"curr" <<curr<<" pre v"<<prev;
	if (curr == tree->getEnabledItemCount() && prev != curr)
	{
		selAllBtn->setText("全不选");
	}
	else if (prev == tree->getEnabledItemCount() && prev != curr)
	{
		selAllBtn->setText("全选");
	}
	if (curr == 0)
	{
		selContLabel->clear();
	}
	else
	{
		selContLabel->setToolTip(QString("已选中%1项").arg(curr));
	}
	okButton->setEnabled(curr != 0);
}

void DownloadDialog::startGetCurrentItemQnList()
{
	qint64 itemId = 0;
	if (tree != nullptr)
	{
		auto item = tree->currentItem();
		if (!item->qnList.empty())
		{
			updateQnComboBox(item->qnList);
			return;
		}
		itemId = item->contentItemId();
	}
	switch (this->contentType)
	{
	case ContentType::LIVE:
		qDebug() << __func__ << " todo ";
		throw QString("un finised");
		break;
	case ContentType::PGC:
		getQnListReply = PgcDownloadTask::getPlayUrlInfo(itemId, 0);
		break;
	case ContentType::UGC:
		throw QString("un finised");
		break;
	case ContentType::PUGV:
		throw QString("un finised");
		break;
	default:
		break;
	}
	connect(getQnListReply, &QNetworkReply::finished, this, &DownloadDialog::getCurrentItemQnListFinished);
	if (tree != nullptr)
	{
		selAllBtn->setEnabled(false);
		tree->setEnabled(false);
	}
	if (getQnListBtn != nullptr)
	{
		getQnListBtn->setEnabled(false);
	}
	qnCombobox->setEnabled(false);
	okButton->setEnabled(false);
}

//return a list qn(in descending order) that exists in a but not in b
static QnList qnListDifferenc(QnList a, QnList b)
{
	QnList ret;
	std::sort(a.begin(), a.end(), std::greater<QnList::Type>());
	std::sort(b.begin(), b.end(), std::greater<QnList::Type>());

	auto it_a = a.constBegin();
	auto it_b = b.constBegin();
	auto end_a = a.constEnd();
	auto end_b = b.constEnd();
	while (it_a != end_a && it_b != end_b)
	{
		auto cmp = (*it_a) - (*it_b);
		if (cmp == 0)
		{
			it_a++;
			it_b++;
		}
		else if (cmp > 0)
		{
			ret.push_back(*it_a);
			it_a++;
		}
		else
		{
			it_b++;
		}
	}
	return  ret;
}
void DownloadDialog::getCurrentItemQnListFinished()
{
	if (tree != nullptr)
	{
		selAllBtn->setEnabled(true);
		tree->setEnabled(true);
		QTimer::singleShot(0, this, [this] {tree->setFocus(); });
	}
	if (getQnListBtn != nullptr)
	{
		getQnListBtn->setEnabled(true);
	}
	qnCombobox->setEnabled(true);
	okButton->setEnabled(true);

	auto reply = getQnListReply;
	getQnListReply = nullptr;
	reply->deleteLater();

	if (reply->error() == QNetworkReply::OperationCanceledError)
	{
		return;
	}
	auto [json, errStr] = Network::Bili::parseReply(reply);
	if (!errStr.isNull())
	{
		QMessageBox::critical(this, "获取画质列表", "获取画质列表失败：" + errStr);
		return;
	}
	QnList qnlist;
	switch (contentType)
	{
	case ContentType::PGC:
	{
		auto data = json[PgcDownloadTask::playUrlInfoDataKey].toObject();
		qnlist = PgcDownloadTask::getQnInfoFromPlayUrlInfo(data).qnList;
		break;
	}
	default:
		break;
	}
	if (tree != nullptr)
	{
		tree->currentItem()->qnList = qnlist;
	}
	updateQnComboBox(std::move(qnlist));

}

void DownloadDialog::selectPath()
{
	auto path = QFileDialog::getExistingDirectory(this, QString(), pathLabel->text());
	if (!path.isNull())
	{
		pathLabel->setText(QDir::toNativeSeparators(path));
	}
}

void DownloadDialog::updateQnComboBox(QnList qnList)
{
	bool onlyOneItem = (tree == nullptr || tree->getEnabledItemCount() == 1);
	QList<std::pair<int, QString>> qnDescList;
	if (contentType == ContentType::LIVE)
	{
		for (auto qn : qnList)
		{
			//qnDescList.emplaceBack(qn,lived)
		}
	}
	else
	{
		for (auto qn : qnList)
		{
			qnDescList.emplaceBack(qn, VideoDownloadTask::getQnDescription(qn));
		}
		if (!onlyOneItem)
		{
			qnList = qnListDifferenc(VideoDownloadTask::getAllPossibleQn(), std::move(qnList));
			for (auto qn : qnList)
			{
				qnDescList.emplaceBack(qn, "*" + VideoDownloadTask::getQnDescription(qn));
			}
		}
	}
	qnCombobox->clear();
	for (auto& [qn, desc] : qnDescList)
	{
		qnCombobox->addItem(desc, qn);
	}
}

void DownloadDialog::addPlayDirect(QBoxLayout* layout)
{
	using std::move;


}
