extends VehicleBody3D

@export var max_rpm = 500
@export var max_torque = 200
@export var steering_angle = 0.5

func _physics_process(delta):
	steering = lerp(steering, Input.get_axis("ui_right", "ui_left") * steering_angle, 5 * delta)
	var acceleration = Input.get_axis("ui_down", "ui_up")
	engine_force = acceleration * max_torque

	# Brake if opposite direction
	# Forward is -Z. If velocity.z is negative (moving forward) and acceleration is negative (pressing back/brake), brake.
	var forward_speed = linear_velocity.dot(-transform.basis.z)

	if acceleration > 0 and forward_speed < -1: # Trying to go forward but moving backward
		brake = 5.0
	elif acceleration < 0 and forward_speed > 1: # Trying to go backward but moving forward
		brake = 5.0
	else:
		brake = 0.0

func _input(event):
	if event.is_action_pressed("ui_accept"):
		# Respawn or reset logic could go here, handled by Main usually
		pass
