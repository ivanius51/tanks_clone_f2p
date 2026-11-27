#include "game/TankTilt.h"

#include <cmath>

#include "core/ResourceIds.h"
#include "model/ConfigNode.h"
#include "services/ConfigRefs.h"

namespace game
{

namespace
{
	const char* const WallPrefix = "tilt_wall_";
	const char* const TurretName = "turret";
	const char* const TurretShadowName = "tilt_turret_shadow";
	const char* const HullPieceName = "hull";
	const char* const TrackNames[] = { "track_left", "track_right" };
	constexpr int WallZ = -2;
	constexpr int TurretShadowZ = 9;
	constexpr float DegreesToRadians = 3.14159265358979f / 180.0f;

	cocos2d::Color3B ReadColor(const model::IConfigNode& InNode)
	{
		return cocos2d::Color3B(static_cast<GLubyte>(InNode.GetItem(0)->ReadNumber().value_or(0.0)), static_cast<GLubyte>(InNode.GetItem(1)->ReadNumber().value_or(0.0)), static_cast<GLubyte>(InNode.GetItem(2)->ReadNumber().value_or(0.0)));
	}

	// Turns a screen-space vector into the coordinates of a node turned by InNodeRotation (cocos turns clockwise).
	cocos2d::Vec2 ToLocal(const cocos2d::Vec2& InScreen, const float InNodeRotation)
	{
		return InScreen.rotateByAngle(cocos2d::Vec2::ZERO, InNodeRotation * DegreesToRadians);
	}

	// How tall the hull looks on screen next to its length: a hull turned sideways is shorter, so its turret rises less.
	float VisibleHeightRatio(const cocos2d::Size& InSize, const float InRotation)
	{
		const float Angle = InRotation * DegreesToRadians;
		return (InSize.height * std::abs(std::cos(Angle)) + InSize.width * std::abs(std::sin(Angle))) / InSize.height;
	}
}

TankTiltSettings TankTiltSettings::Load()
{
	TankTiltSettings Result;
	const model::IConfigNode* const Node = config_refs::GetConfig(ids::configs::TankLook).FindChild("tilt");
	if ( !Node )
	{
		return Result;
	}
	Result.bEnabled = Node->GetValueOr("enabled", Result.bEnabled);
	Result.Squash = Node->GetValueOr("squash", Result.Squash);
	Result.WallShare = Node->GetValueOr("wall", Result.WallShare);
	Result.TurretRaiseShare = Node->GetValueOr("turret_raise", Result.TurretRaiseShare);
	Result.TurretShadowShare = Node->GetValueOr("turret_shadow", Result.TurretShadowShare);
	Result.TurretShadowOpacity = static_cast<unsigned char>(Node->GetValueOr("turret_shadow_opacity", static_cast<float>(Result.TurretShadowOpacity)));
	if ( const model::IConfigNode* const Color = Node->FindChild("wall_color") )
	{
		Result.WallColor = ReadColor(*Color);
	}
	return Result;
}

TankTilt::TankTilt(const TankTiltSettings& InSettings)
	: Settings(InSettings)
{
}

void TankTilt::Update(cocos2d::Sprite& InHull) const
{
	if ( !Settings.bEnabled )
	{
		return;
	}
	const cocos2d::Size Size = InHull.getContentSize();
	const float Rotation = InHull.getRotation();
	const float Angle = -Rotation * DegreesToRadians;
	// A squash along the screen's vertical axis is done in the node's own space: turn back, squash, turn forward.
	cocos2d::Mat4 Back;
	cocos2d::Mat4::createRotationZ(-Angle, &Back);
	cocos2d::Mat4 Scale;
	cocos2d::Mat4::createScale(1.0f, Settings.Squash, 1.0f, &Scale);
	cocos2d::Mat4 Forward;
	cocos2d::Mat4::createRotationZ(Angle, &Forward);
	cocos2d::Mat4 ToCenter;
	cocos2d::Mat4::createTranslation(Size.width * 0.5f, Size.height * 0.5f, 0.0f, &ToCenter);
	cocos2d::Mat4 FromCenter;
	cocos2d::Mat4::createTranslation(-Size.width * 0.5f, -Size.height * 0.5f, 0.0f, &FromCenter);
	InHull.setAdditionalTransform(ToCenter * Back * Scale * Forward * FromCenter);

	// The squash shortens the offsets too, so they are lengthened to land as many screen pixels as the share says.
	const float Lengthen = 1.0f / Settings.Squash;
	UpdateWall(InHull, ToLocal(cocos2d::Vec2(0.0f, -Size.height * Settings.WallShare * Lengthen), Rotation));
	const float RaiseHeight = Size.height * VisibleHeightRatio(Size, Rotation);
	UpdateTurret(InHull, ToLocal(cocos2d::Vec2(0.0f, RaiseHeight * Settings.TurretRaiseShare * Lengthen), Rotation), RaiseHeight);
}

void TankTilt::Clear(cocos2d::Sprite& InHull) const
{
	InHull.setAdditionalTransform(nullptr);
	if ( cocos2d::Node* const Turret = InHull.getChildByName(TurretName) )
	{
		Turret->setAdditionalTransform(nullptr);
	}
	InHull.removeChildByName(TurretShadowName);
	InHull.removeChildByName(std::string(WallPrefix) + HullPieceName);
	for ( const char* const TrackName : TrackNames )
	{
		InHull.removeChildByName(std::string(WallPrefix) + TrackName);
	}
}

cocos2d::Sprite& TankTilt::EnsureWallPiece(cocos2d::Sprite& InHull, const std::string& InName) const
{
	const std::string Name = WallPrefix + InName;
	if ( cocos2d::Sprite* const Existing = static_cast<cocos2d::Sprite*>(InHull.getChildByName(Name)) )
	{
		return *Existing;
	}
	cocos2d::Sprite* const Piece = cocos2d::Sprite::create();
	Piece->setName(Name);
	Piece->setAnchorPoint(cocos2d::Vec2::ZERO);
	Piece->setColor(Settings.WallColor);
	InHull.addChild(Piece, WallZ);
	return *Piece;
}

void TankTilt::UpdateWall(cocos2d::Sprite& InHull, const cocos2d::Vec2& InWallOffset) const
{
	cocos2d::Sprite& Hull = EnsureWallPiece(InHull, HullPieceName);
	Hull.setSpriteFrame(InHull.getSpriteFrame());
	Hull.setPosition(InWallOffset);
	for ( const char* const TrackName : TrackNames )
	{
		cocos2d::Sprite* const Track = static_cast<cocos2d::Sprite*>(InHull.getChildByName(TrackName));
		if ( !Track )
		{
			continue;
		}
		cocos2d::Sprite& Piece = EnsureWallPiece(InHull, TrackName);
		Piece.setSpriteFrame(Track->getSpriteFrame());
		Piece.setPosition(Track->getPosition() + InWallOffset);
	}
}

void TankTilt::UpdateTurret(cocos2d::Sprite& InHull, const cocos2d::Vec2& InRaise, const float InHeight) const
{
	cocos2d::Sprite* const Turret = static_cast<cocos2d::Sprite*>(InHull.getChildByName(TurretName));
	if ( !Turret )
	{
		return;
	}
	cocos2d::Mat4 Lift;
	cocos2d::Mat4::createTranslation(InRaise.x, InRaise.y, 0.0f, &Lift);
	Turret->setAdditionalTransform(&Lift);

	cocos2d::Sprite* Shadow = static_cast<cocos2d::Sprite*>(InHull.getChildByName(TurretShadowName));
	if ( !Shadow )
	{
		Shadow = cocos2d::Sprite::create();
		Shadow->setName(TurretShadowName);
		Shadow->setAnchorPoint(cocos2d::Vec2::ZERO);
		Shadow->setColor(cocos2d::Color3B::BLACK);
		Shadow->setOpacity(Settings.TurretShadowOpacity);
		InHull.addChild(Shadow, TurretShadowZ);
	}
	Shadow->setSpriteFrame(Turret->getSpriteFrame());
	Shadow->setRotation(Turret->getRotation());
	const float DropShare = (Settings.TurretRaiseShare - Settings.TurretShadowShare) / Settings.Squash;
	Shadow->setPosition(Turret->getPosition() + ToLocal(cocos2d::Vec2(0.0f, InHeight * DropShare), InHull.getRotation()));
}

} // namespace game
