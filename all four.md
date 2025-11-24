### **ABSTRACT**

Shell-and-tube heat exchangers are widely used in process industries, where long-term operation is strongly affected by thermal fouling and time-dependent changes in operating conditions. Accurate prediction of heat-transfer performance and pressure drop under both clean and fouled conditions is therefore essential for design, monitoring, and maintenance planning. This project develops and documents a modular mathematical model for a single shell-and-tube heat exchanger that can be run in four modes: steady clean, steady fouled, dynamic clean, and dynamic fouled.

The model uses standard geometric inputs, thermophysical properties of the hot and cold fluids, and operating conditions (flow rates and inlet temperatures). Clean convective heat-transfer coefficients on the tube and shell sides are obtained from Reynolds-Prandtl-Nusselt based correlations, and the overall heat-transfer coefficient is calculated using a series thermal-resistance network including tube-side film, wall conduction, and shell-side film. Fouling is incorporated through both asymptotic (Kern-Seaton type) and linear fouling models, which provide time-dependent fouling resistances and corresponding changes in effective diameters and overall heat-transfer coefficient. Pressure drop on both sides is evaluated using the Darcy-Weisbach equation and appropriate friction-factor correlations.

For the dynamic simulations, lumped energy balances are written on the hot and cold fluid holdups, and the resulting first-order ordinary differential equations are solved numerically to obtain the transient evolution of outlet temperatures, heat duty, effectiveness, overall coefficient, and pressure drop. The results illustrate the expected degradation in thermal performance and increase in hydraulic losses as fouling builds up, as well as the characteristic time response of the exchanger to changes in inlet conditions. The developed framework provides a transparent and extendable basis for heat-exchanger analysis, suitable for use in education, design studies, and future integration with plant monitoring or optimisation tools.

## **LIST OF SYMBOLS**

## **Latin Symbols**

| Symbol | Description | SI Unit |
| --- | --- | --- |
|     | Shell-side flow cross-sectional (free-flow) area | m²  |
|     | Tube-side flow cross-sectional area | m²  |
|     | Internal heat-transfer area on tube side (based on tube inner diameter) | m²  |
|     | External heat-transfer area on tube side / shell side (based on tube outer diameter) | m²  |
|     | Heat capacity rate of cold fluid, | W·K⁻¹ |
|     | Heat capacity rate of hot fluid, | W·K⁻¹ |
|     | Larger of the two capacity rates, | W·K⁻¹ |
|     | Smaller of the two capacity rates, | W·K⁻¹ |
|     | Capacity-rate ratio, | -   |
|     | Thermal capacitance of cold-side control volume, | J·K⁻¹ |
|     | Thermal capacitance of hot-side control volume, | J·K⁻¹ |
|     | Clean shell-side equivalent (hydraulic) diameter | m   |
|     | Shell-side equivalent (hydraulic) diameter with fouling | m   |
|     | Clean tube inner diameter | m   |
|     | Effective tube inner diameter with fouling deposit | m   |
|     | Tube outer diameter | m   |
|     | Darcy friction factor | -   |
|     | Shell-side (outer) convective heat-transfer coefficient | W·m⁻²·K⁻¹ |
|     | Tube-side (inner) convective heat-transfer coefficient | W·m⁻²·K⁻¹ |
|     | Fluid thermal conductivity | W·m⁻¹·K⁻¹ |
|     | Thermal conductivity of cold-side fouling deposit | W·m⁻¹·K⁻¹ |
|     | Thermal conductivity of hot-side fouling deposit | W·m⁻¹·K⁻¹ |
|     | Asymptotic fouling rate constant (cold side) in Kern-Seaton type model | s⁻¹ (or h⁻¹) |
|     | Asymptotic fouling rate constant (hot side) in Kern-Seaton type model | s⁻¹ (or h⁻¹) |
|     | Tube wall thermal conductivity | W·m⁻¹·K⁻¹ |
|     | Tube length (per pass) | m   |
|     | Mass of cold fluid holdup inside exchanger | kg  |
|     | Mass of hot fluid holdup inside exchanger | kg  |
|     | Mass flow rate of cold fluid | kg·s⁻¹ |
|     | Mass flow rate of hot fluid | kg·s⁻¹ |
|     | Number of tubes | -   |
|     | Number of transfer units, | -   |
|     | Nusselt number on shell side (based on ) | -   |
|     | Nusselt number on tube side (based on ) | -   |
|     | Prandtl number of cold fluid | -   |
|     | Prandtl number of hot fluid | -   |
|     | Actual steady-state heat transfer rate | W   |
|     | Instantaneous heat transfer rate at time | W   |
|     | Maximum possible heat transfer rate, | W   |
|     | Cold-side fouling thermal resistance (per reference area) at time | m²·K·W⁻¹ |
|     | Initial cold-side fouling resistance at | m²·K·W⁻¹ |
|     | Asymptotic cold-side fouling resistance at long time | m²·K·W⁻¹ |
|     | Hot-side fouling thermal resistance (per reference area) at time | m²·K·W⁻¹ |
|     | Initial hot-side fouling resistance at | m²·K·W⁻¹ |
|     | Asymptotic hot-side fouling resistance at long time | m²·K·W⁻¹ |
|     | Total thermal resistance at time (films + wall + fouling) | K·W⁻¹ |
|     | Tube wall thermal resistance (referred to chosen area) | m²·K·W⁻¹ or K·W⁻¹ |
|     | Linear fouling growth rate (cold side), | m²·K·W⁻¹·s⁻¹ (or per h) |
|     | Linear fouling growth rate (hot side), | m²·K·W⁻¹·s⁻¹ (or per h) |
|     | Reynolds number of cold fluid on shell side | -   |
|     | Reynolds number of hot fluid on tube side | -   |
|     | Physical (operating) time from start of service | s (or h) |
|     | End time of dynamic simulation | s   |
|     | Numerical integration time step | s   |
|     | Cold-fluid inlet temperature | °C or K |
|     | Cold-fluid outlet temperature | °C or K |
|     | Hot-fluid inlet temperature | °C or K |
|     | Hot-fluid outlet temperature | °C or K |
|     | Overall heat-transfer coefficient (based on ) | W·m⁻²·K⁻¹ |
|     | Clean overall heat-transfer coefficient at design condition | W·m⁻²·K⁻¹ |
|     | Fouled overall heat-transfer coefficient at time | W·m⁻²·K⁻¹ |
|     | Mean velocity of cold fluid in its flow passage | m·s⁻¹ |
|     | Mean velocity of hot fluid in its flow passage | m·s⁻¹ |
|     | Pressure drop on cold (shell) side | Pa  |
|     | Time-dependent pressure drop on cold side (with fouling/dynamics) | Pa  |
|     | Pressure drop on hot (tube) side | Pa  |
|     | Time-dependent pressure drop on hot side (with fouling/dynamics) | Pa  |

### Greek Symbols

| Symbol | Description | SI Unit |
| --- | --- | --- |
|     | Heat-exchanger effectiveness, | -   |
|     | Instantaneous effectiveness at time | -   |
|     | Dynamic viscosity of cold fluid | Pa·s |
|     | Dynamic viscosity of hot fluid | Pa·s |
|     | Density of cold fluid | kg·m⁻³ |
|     | Density of hot fluid | kg·m⁻³ |
|     | Cold-side fouling layer thickness at time | m   |
|     | Hot-side fouling layer thickness at time | m   |
|     | Tube wall thickness (radial) | m   |

## **CHAPTER 1**

## **INTRODUCTION**

### **1.1 Background**

Heat exchangers are key components in thermal systems used to transfer heat between two or more fluids at different temperatures. They are widely used in power plants, refineries, chemical and petrochemical industries, food processing, HVAC systems and many other applications. Among the various types, the **shell-and-tube heat exchanger** is the most commonly used industrial configuration due to its mechanical robustness, flexibility in design, and ability to operate at high temperatures and pressures.

In a typical shell-and-tube heat exchanger, one fluid flows inside a bundle of tubes while the other fluid flows across the outside of the tubes within a shell. The rate of heat transfer between the hot and cold streams depends on the fluid properties, flow configuration, geometry of the exchanger, and the overall heat-transfer coefficient. Over long periods of operation, however, the thermal performance gradually deteriorates due to the formation of **fouling deposits** on the heat-transfer surfaces and changes in operating conditions.

### **1.2 Heat Exchanger Performance and Fouling**

The performance of a heat exchanger is usually characterised by quantities such as:

- Heat duty ,
- Overall heat-transfer coefficient ,
- Effectiveness ,
- Pressure drop on the tube and shell sides.

In design, these quantities are often evaluated under **clean and steady-state** conditions using empirical correlations for convective heat transfer and friction factor. However, in real plants, heat exchangers are rarely perfectly clean. Over time, **fouling layers** form due to mechanisms such as crystallisation, corrosion, particulate deposition, biological growth, or chemical reaction. These deposits introduce additional thermal resistance and may also reduce the effective flow area, thereby:

- Decreasing the overall heat-transfer coefficient,
- Reducing the heat duty,
- Increasing pressure drop and pumping power requirements.

If fouling is not properly understood and managed, it can lead to reduced plant efficiency, higher fuel consumption, unplanned shutdowns, and increased maintenance costs.

### **1.3 Need for Steady and Dynamic Modelling**

Traditional design calculations are mostly **steady-state** and assume clean or uniformly fouled conditions. In practice, however:

- The **degree of fouling evolves with time**, often following asymptotic or approximately linear behaviour.
- **Inlet temperatures and flow rates** of the process streams may vary due to upstream process disturbances or control actions.
- Operating decisions such as **scheduling of cleaning** and **re-rating** of exchangers require knowledge of how performance evolves dynamically.

Therefore, a **combined steady and dynamic model** that captures both:

- **Clean and fouled conditions**, and
- **Transient response** to changes in operating conditions and fouling growth,

is extremely useful for analysis, design verification, and operator training.

### **1.4 Aim of the Present Work**

The main aim of this project is to **develop, implement and demonstrate** a modular mathematical model of a shell-and-tube heat exchanger that can operate in four different modes:

- **Steady Clean Mode**  
  - Prediction of heat duty, outlet temperatures, overall heat-transfer coefficient and pressure drops assuming no fouling.
- **Steady Fouled Mode**  
  - Inclusion of fouling thermal resistances and modified hydraulic diameters to evaluate the degraded steady-state performance under specified fouling conditions.
- **Dynamic Clean Mode**  
  - Time-dependent simulation of the exchanger response to changes in inlet temperatures, flow rates or other operating parameters, assuming surfaces remain clean.
- **Dynamic Fouled Mode**  
  - Full time-dependent simulation with **fouling growth models** (asymptotic or linear) incorporated into the thermal-resistance network, allowing prediction of the evolution of outlet temperatures, heat duty, effectiveness, overall coefficient and pressure drops over the operating period.

### **1.5 Scope and Methodology**

To achieve the above aim, the following scope is considered:

- **Geometry and configuration**  
    A single shell-and-tube heat exchanger with specified tube length, tube diameter, number of tubes and shell-side geometry is modelled. The model is general enough to be adapted to different sizes by changing input parameters.
- **Thermal modelling**
  - Convective heat-transfer coefficients on the tube side and shell side are obtained from **Reynolds-Prandtl-Nusselt** correlations.
  - The **overall heat-transfer coefficient** is computed using a series thermal-resistance network, consisting of tube-side film resistance, tube wall conduction resistance and shell-side film resistance, with fouling resistances added when appropriate.
  - For steady-state cases, algebraic energy-balance equations are solved.
  - For dynamic cases, **lumped energy balances** are written on the hot and cold fluid holdups, leading to first-order ordinary differential equations in time for the outlet temperatures.
- **Fouling modelling**
  - Fouling on the hot side and cold side is represented through **time-dependent fouling resistances** and .
  - Both **asymptotic (Kern-Seaton type)** and **linear fouling growth** models can be used, controlled by appropriate fouling parameters.
  - The impact of fouling on thermal performance (through ) and hydraulic performance (through effective diameters and pressure drop) is included.
- **Hydraulic modelling**
  - Pressure drops on both sides are calculated using **Darcy-Weisbach** type relations with suitable friction-factor correlations as functions of Reynolds number.
  - Changes in flow area or hydraulic diameter due to fouling are reflected in Reynolds number and hence in friction factor and pressure drop.
- **Numerical solution**
  - For dynamic simulations, the governing ordinary differential equations are solved using a suitable time-stepping scheme (e.g. explicit or implicit finite difference) with specified time step and end time .
  - At each time step, updated fluid properties, fouling resistances and overall coefficient are calculated and used to update temperatures and pressure drops.

## **CHAPTER 2**

## **LITERATURE REVIEW**

### **2.1 Overview of Heat Exchangers**

Heat exchangers are devices that transfer thermal energy between two or more fluids at different temperatures without mixing them directly. They are used extensively in power generation, refrigeration and air-conditioning, chemical processing, petroleum refining, food and dairy processing, and many other sectors.

Common industrial heat-exchanger types include:

- **Shell-and-tube heat exchangers**
- **Double-pipe (hairpin) heat exchangers**
- **Plate heat exchangers**
- **Air-cooled heat exchangers**
- **Compact finned-tube heat exchangers**

Among these, the **shell-and-tube heat exchanger (STHE)** is the most widely used in heavy-duty industrial applications due to its mechanical strength, ease of cleaning, ability to handle high pressure and temperature, and flexibility in materials and layout. Because of this, most classical design methods, correlations and fouling data available in handbooks are centred around STHEs.

### 2.2 Shell-and-Tube Heat Exchangers and Design Methods

In a shell-and-tube heat exchanger, one fluid flows inside an array of tubes, while the other flows across the outside of the tubes within a cylindrical shell. Different configurations are possible (single-pass, multi-pass, fixed tubesheet, U-tube, floating head, etc.), but the **basic design tasks** are similar:

- Select geometry (tube diameter, pitch, layout, length, number of tubes, shell diameter).
- Estimate **heat-transfer coefficients** on tube and shell sides.
- Compute the **overall heat-transfer coefficient** .
- Evaluate **heat duty** and **temperature profiles**.
- Check **pressure drop** and pumping-power requirements.

Two classical thermal design approaches are used:

- **Log Mean Temperature Difference (LMTD) method**  
    Used when inlet and outlet temperatures of both fluids are known or specified. The mean driving force is the **log mean temperature difference** , and the heat duty is:

- where is the heat-transfer area and is a correction factor that accounts for flow arrangement (counterflow, parallel flow, shell-and-tube configurations, etc.).

- **Effectiveness-NTU method**  
    Used when one or more outlet temperatures are unknown or when it is convenient to work with non-dimensional groups. The **effectiveness** is defined as:

- and is related to and capacity-rate ratio through relations that depend on the flow arrangement.

For shell-and-tube exchangers, **tube-side** convective heat transfer is usually correlated as a function of **Reynolds number** and **Prandtl number** using Nusselt-number correlations of the general form:

while **shell-side** heat transfer is represented by modified correlations in terms of an equivalent or hydraulic diameter, often based on methods such as Kern's method or the Bell-Delaware method.

### **2.3 Fouling in Heat Exchangers**

**Fouling** refers to the unwanted deposition of solids or formation of layers on the heat-transfer surfaces during operation. This is one of the major problems in industrial heat exchangers. Depending on the system, fouling may arise from:

- **Crystallisation** (precipitation of dissolved salts at surfaces),
- **Particulate deposition** (suspended solids settling or being trapped),
- **Corrosion products** (rust, oxide scales),
- **Biofouling** (microbial growth, algae, slime),
- **Chemical reaction fouling** (polymerisation, coking, etc.),
- **Freezing fouling** (solidification of components at cold surfaces).

The consequences of fouling are:

- Increase in **thermal resistance**, leading to reduced overall heat-transfer coefficient , lower heat duty, and reduced process efficiency.
- Reduction in **flow area** and change in **hydraulic diameter**, causing higher fluid velocity in the remaining open area, increased **pressure drop**, and higher pumping power.
- Possible **maldistribution**, hot spots, and operational instabilities.
- Need for **cleaning and shutdowns**, resulting in maintenance costs and loss of production.

The combined effect is often represented by introducing an additional **fouling thermal resistance** into the series thermal-resistance network. For a single surface, the total resistance becomes:

where and are fouling resistances on the hot and cold sides, respectively. This leads to a reduced effective and can ultimately force operators to schedule chemical or mechanical cleaning to restore performance.

### 2.4 Fouling Models

To include fouling in design and simulation, various **empirical and semi-empirical models** have been proposed. The most common approaches used in engineering practice are:

- **Constant fouling resistance model**  
    The simplest approach is to assume a constant design fouling resistance (for each side), obtained from standards or experience. This value is then added to the thermal-resistance network and treated as time-invariant. While convenient for design, it does not capture the time evolution of fouling.
- **Linear fouling model**  
    In a linear model, the fouling resistance is assumed to grow linearly with time:

- where is the initial fouling resistance (often zero) and is a constant growth rate. This model is simple and often adequate for short operating periods where the fouling has not yet approached an asymptotic limit.

- **Asymptotic (Kern-Seaton type) fouling model**  
    A widely used dynamic fouling model assumes that the fouling resistance increases rapidly at first and then approaches an asymptotic value:

- where is the asymptotic fouling resistance and is a fouling rate constant. This form captures the saturation behaviour observed in many systems: deposition initially dominates, but later removal mechanisms (shear, spalling, dissolution) balance the deposition rate, leading to a limiting resistance.

- **More advanced/threshold models**  
    In more detailed studies, fouling rate may depend on wall temperature, fluid velocity, concentration, or a critical "threshold" condition. However, such models require extensive experimental calibration and are often too complex for routine design work.

For the purpose of this project, **linear** and **asymptotic fouling models** provide a practical balance between realism and simplicity and can be easily incorporated into the overall heat-transfer coefficient and pressure-drop calculations.

### 2.5 Steady-State and Dynamic Modelling of Heat Exchangers

Most classical design methods and handbook correlations are formulated for **steady-state operation**. Under steady conditions, the inlet temperatures, flow rates and physical properties are assumed constant, and the heat exchanger is analysed with algebraic energy balances and correlations for , , and pressure drop.

However, industrial heat exchangers often experience **time-varying conditions**:

- Changes in inlet temperatures due to upstream process fluctuations,
- Changes in flow rates due to control action or process disturbances,
- Gradual fouling build-up and occasional cleaning events,
- Start-up and shutdown transients.

To study such situations, **dynamic models** are used. In many engineering applications, the fluids within the exchanger are treated as **lumped control volumes** with uniform average temperatures. For a single-pass shell-and-tube exchanger, dynamic energy balances can be written, for example, as:

- On the hot-fluid side:
- On the cold-fluid side:

Here, and represent the thermal capacitances of the fluid holdups. The instantaneous heat duty is linked to the current overall coefficient and temperature driving force via NTU-effectiveness or LMTD-type relations. When **time-dependent fouling models** and are included, the model can predict the **evolution of performance** over long operating periods.

Dynamic models are important for:

- Analysing **start-up and shutdown** behaviour,
- Evaluating the **time response** of outlet temperatures to changes in inlet conditions,
- Studying the **impact of fouling growth** on performance and pressure drop,
- Supporting **maintenance scheduling and optimisation**.

### 2.6 Summary and Motivation for Present Work

From the literature, it is clear that:

- Shell-and-tube heat exchangers remain the dominant industrial geometry and are well-supported by correlations for heat transfer and pressure drop.
- Fouling significantly affects exchanger performance and is commonly treated through additional fouling resistances, with constant, linear and asymptotic fouling models being the most useful in practice.
- While many industrial design tools focus on **steady, clean** conditions (with allowance for constant fouling), there is a strong need for **simple but rigorous dynamic models** that can handle both **clean and fouled** operation under time-varying conditions.

However, existing treatments in textbooks and handbooks often describe **steady-state design** and **fouling correlations** separately, without providing a single, self-contained framework that can be easily configured to:

- Steady clean,
- Steady fouled,
- Dynamic clean, and
- Dynamic fouled modes,

using the same set of governing equations and symbols.

The present work addresses this gap by developing a **modular mathematical model** of a shell-and-tube heat exchanger capable of operating in all four modes within a unified framework. The model combines standard heat-transfer and pressure-drop correlations with linear and asymptotic fouling models and is implemented in a form suitable for simulation and educational use. The next chapter presents the detailed **mathematical formulation and governing equations** used in this project.

## **CHAPTER 3**

## **MATHEMATICAL MODELLING**

This chapter presents the complete mathematical model of the shell-and-tube heat exchanger developed in this project. The model is capable of operating in four configurations:

- **Steady clean**
- **Steady fouled**
- **Dynamic clean**
- **Dynamic fouled**

All four modes share a common set of geometric definitions, thermophysical properties, heat-transfer correlations, and hydraulic relations. Fouling and dynamics are then added on top of this base model in a systematic way. The symbols used are consistent with the **List of Symbols** presented earlier and are not repeated here.

### **3.1 Modelling Objectives and Overview**

The objectives of the mathematical model are:

- To predict **heat duty** , **outlet temperatures** , and **overall heat-transfer coefficient** under clean and fouled conditions.
- To estimate **pressure drops** on tube and shell sides.
- To simulate the **time evolution** of the exchanger behaviour when inlet conditions change and when fouling builds up over time.

The modelling approach is based on:

- Standard **Reynolds-Prandtl-Nusselt** correlations for tube-side and shell-side convection.
- A **series thermal-resistance network** for the overall heat-transfer coefficient.
- **Asymptotic** and **linear** fouling models for time-dependent fouling resistance.
- **Darcy-Weisbach** equations for pressure drop.
- **Lumped-parameter energy balances** for dynamic operation, leading to first-order ordinary differential equations (ODEs) in time.

### 3.2 System Description and Basic Assumptions

The model represents a **single shell-and-tube heat exchanger** with one shell and one tube pass, used here as a typical industrial configuration. The following assumptions are made throughout:

- Both fluids remain **single-phase** (no boiling or condensation).
- Fluid properties are evaluated at appropriate **mean temperatures**, and taken constant over each simulation step.
- **Axial conduction** in the fluids and tube wall is neglected.
- Heat losses to the environment are neglected; the exchanger is **adiabatic** to its surroundings.
- On each side, the temperature field is approximated by a **single mean outlet temperature**; axial temperature variation is captured at a lumped level.
- The flow arrangement is treated as **counter-flow** in the thermal design (effectiveness-NTU relations).
- Fouling forms as a **uniform layer** on the tube inner surface (tube side) and on the tube outer surface (shell side).

These assumptions are standard for engineering-level heat-exchanger models and lead to a tractable set of equations suitable for design, analysis, control, and simulation.

### 3.3 Geometry and Heat-Transfer Areas

The key geometric parameters are:

- Tube inner diameter
- Tube outer diameter
- Tube length (per pass)
- Number of tubes
- Shell-side equivalent (hydraulic) diameter (clean)

The **internal tube-side flow area** is

and the **shell-side flow area** is computed from shell diameter, tube pitch and baffle spacing inside the code; conceptually we denote it by

which yields an equivalent cross-flow area used in Reynolds-number and pressure-drop calculations.

The **heat-transfer areas** are defined as:

- Internal (tube-side) heat-transfer area
- External (shell-side) heat-transfer area (based on outer diameter)

All overall-coefficient calculations in this project are carried out **on the basis of the outer area** .

### 3.4 Thermophysical Properties and Dimensionless Groups

For each fluid (hot and cold), the following properties are required:

- Density (kg·m)
- Dynamic viscosity (Pa·s)
- Specific heat at constant pressure (J·kg·K)
- Thermal conductivity (W·m·K)

In the implementation, these are supplied either directly or via a property module that evaluates them at the **mean fluid temperature**.

The **Reynolds number** for the tube side is

where is the mean fluid velocity inside the tubes and is the hot-side mass flow rate.

For the shell side, the Reynolds number is defined using the equivalent diameter and mean cross-flow velocity :

The **Prandtl numbers** for hot and cold fluids are

These dimensionless groups determine the flow regime (laminar vs. turbulent) and enter all convective-heat-transfer and friction-factor correlations used later.

### 3.5 Convective Heat-Transfer Coefficients

The local convective heat-transfer coefficients on each side, (tube side) and (shell side), are obtained from **Nusselt-number correlations** of the general form

#### 3.5.1 Tube-Side Convection

For fully developed **laminar flow** in a circular tube with **constant heat flux** boundary condition, the classical result is

leading to

For **turbulent flow**, the model uses the well-known **Dittus-Boelter correlation**:

where for heating of the fluid and for cooling. In this work, the heating case is assumed on the tube side, so . The tube-side coefficient then becomes

The correlation is applied in its usual validity range (fully turbulent flow, , and moderate Prandtl numbers).

#### 3.5.2 Shell-Side Convection

The shell-side flow over the tube bundle is treated as **cross-flow over a bank of tubes**. A Zhukauskas-type tube-bank correlation is used:

where the constants depend on the flow regime and tube arrangement. In the implemented model, typical values used are

which are appropriate for turbulent flow of liquids across staggered tube banks. The shell-side coefficient is then

In fouled operation, the equivalent diameter is modified to an effective value to account for the reduced flow passage due to fouling, as described in Section 3.8.

### 3.6 Overall Heat-Transfer Coefficient (Clean)

The overall heat-transfer coefficient (based on outer tube area ) is obtained from a **series thermal-resistance network**. For clean operation (no fouling), the resistances are:

- Tube-side convection:
- Tube-wall conduction (cylindrical wall):
- Shell-side convection:

The **total thermal resistance** referred to the outer area is

By definition,

or more conveniently, since is already based on ,

where is the wall resistance converted to a per-unit-area basis on . In the implementation, this conversion is done consistently so that all resistances are combined correctly.

### 3.7 Steady-State Thermal Model (Clean)

For any given operating point, the **heat-capacity rates** of the hot and cold fluids are

The smaller and larger of these are

and the **capacity-rate ratio** is

The **Number of Transfer Units** (NTU) is defined as

For a **counter-flow** heat exchanger, the **effectiveness** is given by

with the special case handled by the usual limit expression.

The **maximum possible heat transfer** is

and the **actual heat transfer rate** in steady clean operation is

The outlet temperatures follow from simple energy balances:

This completes the **steady clean effectiveness-NTU model**, which is used as the base case and also as the starting point for fouled and dynamic simulations.

### 3.8 Fouling Model: Resistance, Thickness and Effective Geometry

In fouled operation, additional thermal resistances appear on both tube and shell sides, and the effective flow geometries change due to deposit build-up.

#### 3.8.1 Fouling Resistance as a Function of Time

For each side ( for hot/tube, for cold/shell), two alternative models are used:

**(a) Asymptotic fouling model (Kern-Seaton type)**

where

- is the initial fouling resistance at ,
- is the asymptotic fouling resistance,
- is a rate constant.

This model captures the rapid initial growth followed by an approach to a limiting fouling resistance.

**(b) Linear fouling model**

where is a constant fouling growth rate. This is appropriate when fouling appears approximately linear over the operating period of interest.

#### 3.8.2 Conversion from Fouling Resistance to Deposit Thickness

If the deposit is treated as a solid layer of effective thermal conductivity , then its thermal resistance per unit area is

Thus, for a given , the corresponding **fouling layer thickness** on tube or shell side is

Typical values of for inorganic and organic deposits lie in the range 0.1-1 W·m·K. In this project, representative values are chosen consistent with literature data.

#### 3.8.3 Effective Diameters in the Presence of Fouling

On the **tube side**, fouling forms on the inner surface, reducing the free flow diameter. The **effective inner diameter** is

and the corresponding flow area and velocity become

On the **shell side**, fouling on the outer tube surface effectively increases the tube outer diameter and reduces the free cross-flow area. The clean equivalent diameter is

and the **effective fouled equivalent diameter** is

These effective diameters are substituted back into the Reynolds-number and Nusselt-number calculations, so that **fouling affects both heat transfer and pressure drop** via geometry changes as well as through the added thermal resistance.

### 3.9 Overall Heat-Transfer Coefficient (Fouled)

With fouling on both sides, the series resistance network extends to include and . Referred to the outer area , the total resistance is

where and are the fouling resistances on shell and tube side respectively, converted to the same area basis.

The **fouled overall coefficient** is then

It is often convenient to write this as a modification of the clean coefficient:

where is an equivalent fouling resistance for the entire exchanger, referred to the same area as . In the present model, is re-computed explicitly from the individual resistances to maintain transparency.

### 3.10 Pressure-Drop (Hydraulic) Model

The hydraulic model is based on the **Darcy-Weisbach equation**, with separate friction-factor correlations on tube and shell sides.

#### 3.10.1 Tube-Side Pressure Drop

The **Darcy friction factor** for internal tube flow is modelled as:

- Laminar ():
- Turbulent (smooth-tube, e.g. Blasius correlation):

The pressure drop along the tubes is then

where is the equivalent flow length (including passes) and collects minor-loss coefficients (entrance, exit, headers, bends).

In fouled operation, the effective diameter is used, increasing the velocity and hence the frictional loss.

#### 3.10.2 Shell-Side Pressure Drop

On the shell side, pressure drop is also modelled with Darcy-Weisbach logic, but using the equivalent diameter and an appropriate friction-factor correlation for cross-flow over tube banks:

The friction factor is obtained from empirical relations for tube banks as a function of . Fouling reduces , which increases and typically leads to higher shell-side .

The **maximum allowable pressure drops** on tube and shell sides are input as design limits; these are checked during simulation to assess whether a given operating point is hydraulically feasible.

### 3.11 Dynamic Model (Clean and Fouled)

To simulate **time-dependent behaviour**, the exchanger is modelled as two **lumped thermal capacitances**: one for the hot side and one for the cold side. Each side has a stored energy that changes due to mass flow and heat exchange.

#### 3.11.1 Thermal Capacitances

The thermal capacitances (hot and cold) are

where are the masses of fluid holdup on hot and cold sides respectively. These are determined from the volume of the exchanger and the fluid densities.

#### 3.11.2 Dynamic Energy Balances

Let and denote the **instantaneous outlet temperatures**. The inlet conditions may vary in time.

The **hot-side energy balance** is

and the **cold-side energy balance** is

The **instantaneous heat transfer rate** is expressed as

where is the **time-dependent overall coefficient** (clean or fouled, as relevant) and is a chosen driving temperature difference. In this project, is taken as the difference between the outlet temperatures

which is a common approximation in lumped models.

When fouling is enabled, the fouling resistances are evaluated at each time using the models in Section 3.8, converted to thicknesses and effective diameters, and used to update and the hydraulic parameters.

#### 3.11.3 Numerical Time Integration

The coupled ODE system is solved numerically using a **fixed-step explicit method**. For a time step , the forward-Euler updates are

At each time step:

- The current time and operating conditions are known.
- Fouling resistances are evaluated (if fouling is enabled).
- Effective diameters and areas are updated.
- Convective coefficients and are computed.
- The heat duty is calculated.
- The outlet temperatures are advanced to .
- Pressure drops are updated using the hydraulic model.

This procedure yields time profiles of all relevant variables.

### 3.12 Operating Modes

The same mathematical framework can be configured to represent four operating modes:

- **Steady clean**
  - Fouling resistances set to zero: .
  - No time dependency; steady NTU-effectiveness equations (Section 3.7) used.
- **Steady fouled**
  - Fouling resistances evaluated at a chosen operating time .
  - Effective diameters updated accordingly.
  - Steady NTU-effectiveness equations used with the fouled .
- **Dynamic clean**
  - Fouling disabled; for all .
  - ODE system (Section 3.11) integrated in time to obtain , and pressure drops under varying inlet conditions.
- **Dynamic fouled**
  - Fouling models enabled; evolve according to asymptotic or linear laws.
  - , effective diameters, Reynolds numbers, and pressure drops become time-dependent.
  - ODE system integrated to capture both **fast thermal dynamics** and **slow fouling-induced degradation**.

Switching between modes is therefore achieved by enabling/disabling fouling and choosing steady-state or dynamic solution of the governing equations.

### 3.13 Summary and Limitations

This chapter has formulated a complete mathematical model for the shell-and-tube heat exchanger used in the project. The model includes:

- Geometric definitions and heat-transfer areas,
- Dimensionless groups and convective correlations,
- Clean and fouled overall heat-transfer coefficients,
- Pressure-drop relations on tube and shell sides,
- Fouling-growth models and their impact on geometry and thermal resistance,
- Lumped dynamic energy balances leading to a two-state ODE system for outlet temperatures.

The main **limitations** of the model are:

- One-dimensional, lumped representation of each side; axial and radial gradients within the exchanger are not resolved.
- Single-phase fluids only; phase change and condensation/boiling are not considered.
- Use of standard empirical correlations (Dittus-Boelter, Zhukauskas, Blasius) which have known validity ranges; accuracy may decrease outside those ranges.
- Fouling assumed uniform and isotropic on each side; local fouling effects and non-uniform distribution are neglected.
- Wall thermal capacitance has been neglected; only fluid holdups are modelled dynamically.

Despite these simplifications, the model captures the **key physical phenomena** governing the thermal and hydraulic performance of shell-and-tube heat exchangers and is detailed enough to support design studies, performance evaluation, dynamic simulation, and control-system development in subsequent chapters.

## **CHAPTER 4**

## **SIMULATION METHODOLOGY AND IMPLEMENTATION**

This chapter describes how the mathematical models from Chapter 3 are used to perform four kinds of simulations:

- Steady Clean Simulation
- Steady Fouled Simulation
- Dynamic Clean Simulation
- Dynamic Fouled Simulation

For each simulation type, the **inputs**, **outputs**, and the **step-by-step calculation procedure** are presented. Finally, the mapping of these concepts to the C++ implementation (modules Thermo, Fouling, Hydraulics, Model, and Simulator) is discussed.

### 4.1 Common Inputs and Basic Data

All simulations share a common set of **base inputs**:

- **Geometry**
  - Tube inner diameter
  - Tube outer diameter
  - Tube length
  - Number of tubes
  - Shell-side equivalent diameter (computed from shell diameter, baffle spacing, pitch, etc.)
  - Tube wall thermal conductivity and thickness
- **Fluid Properties**  
    (Evaluated at suitable mean temperatures)
  - Hot side (tube side): , , ,
  - Cold side (shell side): , , ,
- **Operating Conditions**
  - Hot-side mass flow rate and inlet temperature
  - Cold-side mass flow rate and inlet temperature
- **Flow Arrangement**
  - Counter-flow arrangement is assumed in the effectiveness-NTU relations.
- **Design Limits (for hydraulics)**
  - Maximum allowable tube-side pressure drop
  - Maximum allowable shell-side pressure drop

Additional inputs specific to each simulation type (e.g. fouling parameters, time step, simulation horizon) are described under the relevant sections.

### 4.2 Simulation Type 1 - Steady Clean Operation

#### 4.2.1 Inputs

In steady clean mode, the exchanger is assumed free of fouling:

Required inputs:

- All **common inputs** from Section 4.1.
- Correlations and constants for:
  - Tube-side convection (Dittus-Boelter).
  - Shell-side convection (tube-bank correlation).
  - Pressure drop on tube and shell sides.

#### 4.2.2 Outputs

The main outputs are:

- Hot outlet temperature
- Cold outlet temperature
- Heat duty
- Overall heat-transfer coefficient (clean)
- Tube-side and shell-side pressure drops ,

#### 4.2.3 Calculation Procedure (Step-by-Step)

- **Compute Heat-Transfer Areas**
- **Compute Reynolds and Prandtl Numbers**
  - Tube side:
  - Shell side:
  - where is the mean shell-side cross-flow velocity obtained from the shell geometry and .
- **Evaluate Convective Coefficients**
  - Tube side:
    - If : laminar .
    - If : turbulent with Dittus-Boelter:
  - Shell side (tube bank correlation):
  - with empirical constants chosen for the given geometry and flow regime.
- **Compute Wall Resistance**  
    Tube-wall conduction resistance for a cylindrical wall:
- **Compute Clean Overall Heat-Transfer Coefficient**  
    Total resistance (clean), referred to the outer area:

- Then

- **Compute Capacity Rates and NTU**

- **Effectiveness and Heat Duty**  
    For counter-flow:

- **Outlet Temperatures**
- **Pressure Drops**
  - Tube side:

  - Shell side:  
        analogous expression with shell-side friction factor , equivalent diameter , and velocity .

This is exactly what the C++ Thermo and Hydraulics classes compute in the **steady clean** case.

### 4.3 Simulation Type 2 - Steady Fouled Operation

Steady fouled simulation extends the steady clean model by including fouling resistance and its effect on geometry and performance.

#### 4.3.1 Additional Inputs

In addition to all steady clean inputs, we require **fouling parameters** for each side:

- Initial fouling resistance .
- Asymptotic resistance and rate constant (for asymptotic model),  
    or linear growth rate (for linear fouling).
- Effective thermal conductivity of deposit (tube-side fouling), (shell-side fouling).
- A chosen **operating time** at which fouled performance is to be evaluated.

In this simulation type, fouling is assumed **time-invariant at** ; dynamics are not yet considered.

#### 4.3.2 Outputs

Same as steady clean, but **with fouling**:

- Fouling resistance and thickness at , e.g. , .

#### 4.3.3 Calculation Procedure (Step-by-Step)

- **Calculate Fouling Resistance at**
  - Asymptotic model:
  - or linear model:
- **Convert Resistance to Thickness**
- **Update Effective Diameters**
  - Tube side:
  - Shell side:
  - (conceptually representing reduced free gap due to deposits).
- **Recompute Reynolds, Nusselt, and Convective Coefficients**  
    Use the **same formulas** as in steady clean, but with **fouled diameters** and updated velocities:
- **Recompute Overall Coefficient**  
    Total resistance now includes fouling:

- Thus,

- **Repeat Effectiveness-NTU Calculation**  
    Exactly as in Section 4.2.3, but with replaced by . This yields new , , and .
- **Recompute Pressure Drops**  
    Use updated diameters and in the tube-side and shell-side pressure-drop formulas to obtain and .

This simulation shows how performance degrades and pressure drop increases for a **particular fouling level** (i.e. a fixed operating time).

### 4.4 Simulation Type 3 - Dynamic Clean Operation

Dynamic clean simulation studies the **time response** of the exchanger to changes in inlet conditions (or other disturbances), assuming the surfaces remain clean.

#### 4.4.1 Additional Inputs

Beyond the steady inputs, we need:

- **Time-step** (s).
- **Total simulation time** (s).
- **Hot- and cold-side holdup masses** .
- Time-varying **inlet conditions** (if any):  
    .

In your implementation, realistic disturbances are added via small sinusoidal variations in inlet temperatures and mass flow rates to emulate real plant behaviour.

#### 4.4.2 Outputs

Typical outputs are **time series** of:

- ,
- ,

#### 4.4.3 Governing Equations (Clean Dynamic Model)

Thermal capacitances:

Dynamic energy balances (no fouling, so = clean based on current flow and temperatures):

- **Hot side:**
- **Cold side:**

The instantaneous heat transfer rate is approximated as

**Assumption:** The difference between outlet temperatures is used as the lumped driving temperature difference. This keeps the model simple and is a standard approximation for control-oriented dynamic models.

#### 4.4.4 Numerical Integration (Explicit Euler)

For a time step :

,

At each step:

- Read current inlet conditions .
- Compute , and as in steady clean (Section 4.2), using current flows and temperatures.
- Compute using the outlet-temperature difference.
- Update and with the Euler formulas.
- Compute instantaneous pressure drops from current flows and geometry.
- Advance time and repeat until .

**Assumptions/Remarks:**

- must be chosen sufficiently small to ensure numerical stability (for explicit Euler) and to capture relevant dynamics.
- Fluid properties are assumed constant over each time step.
- No fouling accumulation is considered in this mode; for all .

This is exactly what your Simulator class does when fouling is disabled.

### 4.5 Simulation Type 4 - Dynamic Fouled Operation

Dynamic fouled simulation combines **time-dependent fouling models** with the **dynamic energy balances**, allowing the study of long-term degradation and transient responses simultaneously.

#### 4.5.1 Additional Inputs

In addition to all inputs for dynamic clean, we require:

- Fouling parameters for tube and shell sides (same as in steady fouled):  
    , , for asymptotic model, or for linear model.
- Effective deposit thermal conductivities , .
- Optionally, separate fouling models for hot and cold sides (e.g. faster fouling on hot side).

#### 4.5.2 Outputs

Time series of:

- Fouling resistance and thickness

These are used to analyse how performance deteriorates over months or years of equivalent operating time and to evaluate potential cleaning schedules or control strategies.

#### 4.5.3 Coupled Dynamic-Fouling Algorithm

At each time step :

- **Update Fouling Resistance**
  - Asymptotic:
  - Linear:
- **Get Thickness and Geometry**

- **Recompute Velocities, Reynolds, Nusselt, and**  
    Using the fouled diameters and current mass flow rates.
- **Compute Fouled Overall Coefficient**  
    Using total resistance:

- **Evaluate Heat Duty**
- **Integrate Energy Balances**  
    Same dynamic ODEs as in Section 4.4, but with time-varying and .
- **Update Pressure Drops**  
    With fouled diameters and , recompute .
- **Advance Time**  
    and repeat.

**Key assumptions:**

- Fouling changes slowly compared to thermal dynamics. Within a single time step, it is reasonable to treat as constant.
- Fouling is symmetric or split in a predefined ratio between shell and tube side (your implementation currently splits total fouling resistance 50-50 between shell and tube for simplicity).

### 4.6 Software Implementation Structure

The above methodology is implemented in C++ as a set of modular classes:

- **Types** - Holds all fundamental data structures: Fluid, Geometry, OperatingPoint, FoulingParams, Limits, State.
- **Thermo** - Implements all steady-state thermal calculations:
  - Heat-transfer coefficients .
  - Overall (clean and fouled).
  - ε-NTU method to get .
- **Fouling** - Implements asymptotic and linear fouling models and converts resistance to thickness.
- **Hydraulics** - Computes pressure drops on tube and shell sides using Darcy-Weisbach-type relations and friction-factor correlations.
- **Model** - Combines Thermo, Hydraulics, and Fouling to compute a consistent State for any operating point and time .
- **Simulator** - Integrates the dynamic model in time for clean or fouled cases using explicit Euler, implements disturbances, and generates time series for plotting.

Higher-level modules (not covered in detail here) handle PID control, online parameter estimation, and the graphical user interface ("HeatXTwin Pro").

### 4.7 Summary

In this chapter, four simulation modes have been defined and linked directly to the mathematical model:

- **Steady Clean** - baseline design performance.
- **Steady Fouled** - performance at a chosen fouling level.
- **Dynamic Clean** - transient response to operating changes.
- **Dynamic Fouled** - combined effect of transients and time-varying fouling.

The underlying **assumptions**, required **inputs**, expected **outputs**, and **algorithmic steps** have been clearly stated. This provides the bridge between the theoretical model (Chapter 3) and the actual C++ implementation used in the HeatXTwin Pro simulation environment. Subsequent chapters can now focus on **control design, parameter estimation, and results/analysis** based on these simulation capabilities.

## CHAPTER 5

## CONTROL AND ESTIMATION (PID + RLS)

This chapter describes how closed-loop control and online parameter estimation can be applied to the shell-and-tube heat exchanger model developed in Chapters 3 and 4. The goal is to:

- **Control** a key process variable (for example, cold-side outlet temperature) using a **PID controller**.
- **Estimate** slowly varying plant parameters (for example, overall heat-transfer coefficient or fouling resistance) using a **Recursive Least Squares (RLS)** estimator.

Both are implemented in the project as small, reusable C++ classes:

- hx::ControllerPID - a discrete-time PID controller with anti-windup and rate limiting.
- hx::EstimatorRLS - a scalar RLS estimator with exponential forgetting, suitable for estimating or a scalar fouling parameter.

### 5.1 Motivation for Control and Estimation

In practical operation, a heat exchanger rarely runs at a fixed, ideal point:

- **Inlet conditions** vary due to upstream disturbances and control actions elsewhere in the plant.
- **Fouling** gradually increases the total thermal resistance and changes the effective , reducing heat duty and altering outlet temperatures.
- **Pressure drop** and **pumping power** may also change with fouling and flow adjustments.

From a process point of view, typical **control objectives** are:

- Maintain the **cold-side outlet temperature** at a specified **setpoint** .
- While respecting constraints on **pressure drop**, **flow rates**, and **equipment limits**.

At the same time, operators and advanced control systems benefit from **online estimation** of:

- The **effective overall heat-transfer coefficient** .
- Or an equivalent scalar **fouling parameter** (for example, a dominant fouling resistance ).

Such estimates are useful for:

- Early detection of **performance degradation**,
- **Adaptive control** (retuning the controller based on current process gain),
- Planning **cleaning or maintenance** based on actual fouling progression rather than fixed schedules.

### 5.2 Control Structure and Controlled/Manipulated Variables

In this project, the control structure is designed to be simple and representative:

- **Controlled variable (CV)**:
  - Cold-side outlet temperature (or, equivalently, the heat duty if desired).
- **Manipulated variable (MV)**:
  - Cold-side mass flow rate , OR
  - Hot-side mass flow rate , OR
  - A valve opening signal that is proportional to one of the flows.

(Which variable is chosen as MV is defined at the UI / supervisory level. The PID class itself is generic and can be connected to any scalar input.)

- **Disturbances**:
  - Variations in , , , upstream pressure, etc.
  - Gradual change of due to fouling.

The typical **single-loop feedback structure** is:

where is the controller output (e.g., valve command or flow setpoint).

### 5.3 PID Control Law

#### 5.3.1 Continuous-Time PID Concept

In ideal continuous form, a PID controller is:

where:

- is the control error,
- is the proportional gain,
- is the integral gain,
- is the derivative gain.

The PID aims to:

- **Proportional term**: react immediately to deviations.
- **Integral term**: eliminate steady-state offset.
- **Derivative term**: anticipate changes (rate of error), improving stability and damping.

However, digital implementation requires a discrete-time approximation and practical modifications such as saturation limits, rate limits, and anti-windup.

#### 5.3.2 Discrete-Time Implementation Used

In the project, the controller is implemented as a small class:

classControllerPID {  
public:  
ControllerPID(double kp, double ki, double kd,  
double umin, double umax, double rateLimit);  
doubleupdate(double sp, double y, double dt);  
private:  
double kp_, ki_, kd_;  
double umin_, umax_, rate_;  
double u_, ui_, prevErr_;  
};

At each time step with duration , the control update does the following:

- **Compute error**

- where sp is the setpoint and y is the measured output (e.g. ).

- **Update integral term**  
    A simple rectangular rule is used:
- **Anti-windup clamp on integral term**  
    To avoid integral windup when the actuator saturates, the integral term is restricted between the same minimum and maximum as the controller output:
- **Derivative term**  
    Discrete derivative on the error:

- where is a small number to avoid division by zero.

- **Raw controller output** (before rate limit and saturation):
- **Rate limit**  
    The change in output is limited to a maximum slope rateLimit:

- where is the allowed rate (units of MV per second). The new output is then

- **Final output saturation**  
    Finally, the MV is clamped inside actuator limits:

This discrete law is exactly what ControllerPID::update() computes internally.

#### 5.3.3 Assumptions and Practical Considerations

- **Sample time** is chosen equal to the simulation time step used by the Simulator to avoid timing mismatch.
- **Measurement noise** is not explicitly filtered here; in practice, a low-pass filter on the measured signal or on the derivative term is common.
- **Valve/flow dynamics** are assumed fast relative to heat-exchanger dynamics, so the manipulated variable (e.g. mass flow) is taken to follow the controller output instantaneously, within rate and saturation limits.
- The PID gains are tuned manually or via simple tuning rules (e.g. trial-and-error based on test responses of the model) to achieve acceptable rise time and damping.

### 5.4 RLS Estimation of a Scalar Parameter

#### 5.4.1 Motivation and Modelling Idea

Because fouling and other unmodelled effects change the effective overall heat-transfer coefficient , the **true process gain** between the manipulated variable and the controlled variable is not constant. If a controller is tuned assuming a "clean" , its performance may degrade significantly as fouling increases.

One way to address this is to **estimate online** a scalar parameter representing:

- The current **overall conductance** , or
- A dominant **fouling resistance parameter** .

A very common approach is to cast the problem in the form of a **linear regression**:

where:

- is a measured quantity (e.g. heat duty ),
- is a known regressor (e.g. a temperature driving force),
- is an unknown scalar parameter to be estimated (e.g. ),
- is measurement noise.

For example, in steady or slowly varying operation:

As data arrives over time, the **Recursive Least Squares (RLS)** algorithm updates the current estimate without storing all past data.

#### 5.4.2 RLS Algorithm Used

The implemented RLS estimator hx::EstimatorRLS maintains:

- : current estimate of the parameter,
- : covariance (uncertainty) of the estimate,
- : forgetting factor (0 < ≤ 1).

The update equations at each sample are:

- **Gain computation**
- **Parameter update**
- **Covariance update**

The forgetting factor ensures that older data has diminishing influence:

- : no forgetting (pure recursive least squares, good for time-invariant parameters).
- : exponential forgetting, suitable when the parameter (e.g. fouling) **changes slowly in time**.

EstimatorRLS constructor takes the initial guess , initial covariance , and .

#### 5.4.3 Choice of Measurement and Regressor

The project keeps EstimatorRLS generic. Typical choices are:

- **Estimating** :
  - measured or computed from inlet/outlet temperatures and flows (model-based).
  - (for example, counter-flow mean or outlet-difference).
  - Then .
- **Estimating an equivalent fouling resistance** :
  - and chosen to reflect the relationship between measured and ; for instance, using  
        , rearranged.

In all cases, the assumption is that **measurement noise is zero-mean**, and that the relationship between and is approximately linear for the chosen scalar parameter.

#### 5.4.4 Assumptions and Limitations

- The RLS estimator assumes the underlying parameter changes **slowly** compared to the sampling rate.
- The regressor must be **persistently exciting** over time to avoid ill-conditioning (i.e., it should not be essentially constant).
- If the process operates at nearly constant conditions (small variation in ), parameter estimation becomes poor or very slow.
- RLS here is **scalar**; extension to vector parameters (e.g. simultaneous estimation of and a fouling time constant) would require a matrix RLS formulation.

### 5.5 Integration of PID Control and RLS Estimation

Although the PID controller and RLS estimator are independent modules, they can be combined in several useful ways.

#### 5.5.1 Basic Feedback + Monitoring

The simplest integration is:

- Use **PID** to regulate using or as the MV.
- In parallel, use **RLS** to estimate (or ) from measured and temperature differences.

The PID loop runs at each simulation step:

- Measurement: .
- Error: .
- PID update: .
- Map to a mass flow rate within allowed limits.

The RLS loop uses the same or slightly slower sampling:

- Compute or measure from inlet/outlet temperatures and flows.
- Choose .
- Call EstimatorRLS::update(y_k, phi_k) to obtain .

The estimated can then be:

- Logged and plotted over time to show how the **effective UA decays with fouling**.
- Compared to the fouling-model prediction for validation.
- Used to trigger **alarms** or **maintenance suggestions** when the estimate crosses thresholds.

#### 5.5.2 Adaptive or Gain-Scheduled Control (Conceptual)

While the current implementation keeps PID gains fixed, the RLS estimate can conceptually support **adaptive control**:

- If falls below a certain fraction of its design value, the controller gains could be **adjusted** to maintain satisfactory closed-loop performance.
- For example, as the process gain from to changes with fouling, the proportional gain might be reduced to prevent oscillations.

Such adaptive strategies can be added later without changing the structure of ControllerPID or EstimatorRLS-only the way gains are chosen over time.

### 5.6 Connection to the Simulation Environment

In the overall C++ simulation:

- Simulator and Model encapsulate the **plant** (heat exchanger with fouling and hydraulics).
- ControllerPID acts as the **controller block** in a feedback loop.
- EstimatorRLS acts as an **observer/estimator** block, fed by measured or computed signals.

A typical simulation loop (conceptually) is:

- At time , obtain and other outputs from Simulator::step().
- Compute control error and update PID:
- Convert into a new value for the manipulated mass flow rate (within design limits) and pass that as input for the next Simulator::step().
- Compute or measure heat duty and driving temperature difference .
- Update RLS estimate:
- Store for plotting/analysis and check if any maintenance thresholds are crossed.
- Advance time and repeat until the end of the simulation horizon.

This structure mirrors a real plant where the heat exchanger is under feedback control and where an online estimator provides updated information about its health and performance.

### 5.7 Summary

This chapter has presented the **control and estimation layer** built on top of the heat exchanger model:

- A **discrete PID controller** with:
  - Proportional, integral, and derivative actions,
  - Integral anti-windup via clamping,
  - Rate limiting and saturation of the output.
- A **scalar Recursive Least Squares estimator** with forgetting factor:
  - Suitable for estimating an effective or dominant fouling parameter,
  - Updated online using measured or computed process data.

The combination of these two elements allows:

- **Closed-loop regulation** of key thermal variables such as outlet temperature.
- **Online tracking** of performance degradation due to fouling.

Together with the steady and dynamic models from Chapters 3 and 4, this provides a complete framework for simulation, analysis, and control of a shell-and-tube heat exchanger, forming the technical core of the HeatXTwin Pro project.

## **CHAPTER 6**

## **SIMULATION RESULTS AND DISCUSSION**

This chapter presents representative simulation results obtained from the heat-exchanger model and the control and estimation framework described in the previous chapters. The objective is to:

- Demonstrate the behaviour of the exchanger under **steady clean** and **steady fouled** conditions.
- Illustrate the impact of **fouling progression** on thermal and hydraulic performance.
- Analyse **dynamic responses** to disturbances in inlet conditions.
- Evaluate the performance of the **PID controller** for outlet-temperature regulation.
- Show the capability of the **RLS estimator** to track changes in an effective overall conductance or fouling-related parameter.

Unless otherwise stated, all simulations use the same base geometry and fluid data described in Chapters 3 and 4. Only the operating conditions and fouling parameters are varied between cases.

### 6.1 Definition of Base Case and Test Scenarios

#### 6.1.1 Base Geometry and Fluids

The base-case shell-and-tube exchanger geometry is defined by:

- Tube inner diameter , outer diameter , and length .
- Number of tubes .
- Shell diameter and baffle configuration, which determine the shell-side equivalent diameter and flow area.
- Tube-wall material with thermal conductivity and wall thickness .

The hot fluid flows inside the tubes and the cold fluid flows on the shell side. Typical thermophysical properties for the selected liquids (e.g. water or a process stream) are used at appropriate mean temperatures.

#### 6.1.2 Operating Conditions

For all simulations, the following quantities are specified at the inlet:

- Hot side: mass flow rate and inlet temperature .
- Cold side: mass flow rate and inlet temperature .

A **nominal operating point** is defined as the set of flows and inlet temperatures that represent the intended design condition. Around this nominal point, the model is exercised in four modes:

- **Steady Clean** - No fouling, constant inlet conditions.
- **Steady Fouled** - Fixed fouling level corresponding to a given operating time.
- **Dynamic Clean** - Time-varying inlet conditions, no fouling.
- **Dynamic Fouled** - Time-varying inlet conditions with time-dependent fouling.

In the dynamic simulations, the time step and total simulation time are chosen such that both fast transients (thermal response) and slow changes (fouling accumulation) can be observed.

#### 6.1.3 Fouling Scenarios

Two fouling representations are considered:

- **Asymptotic model**: rapid initial buildup followed by saturation at a limiting resistance .
- **Linear model**: approximately linear increase in fouling resistance over the considered time horizon.

Separate parameters may be specified for tube and shell sides to reflect different fouling tendencies. For comparison, a purely clean case () is also simulated.

### 6.2 Steady Clean Results

The steady clean simulations provide a reference for thermal and hydraulic performance.

#### 6.2.1 Thermal Performance at Design Point

At the nominal operating point, the model computes:

- **Clean overall heat-transfer coefficient** .
- **Heat duty** .
- **Cold- and hot-side outlet temperatures** and .
- **Effectiveness** .

A representative summary can be reported in a table:

**Table 6.1**: Steady Clean Design Performance  
(Example structure - fill with your values)

- Inlet temperatures
- Outlet temperatures
- Heat duty
- Overall coefficient
- Effectiveness

From these results, it is usually observed that:

- The outlet temperatures are consistent with expected energy balances.
- The effectiveness lies in a realistic range for the chosen NTU and capacity-rate ratio .
- The computed is consistent with design correlations for the specific geometry and fluids.

#### 6.2.2 Sensitivity to Flow Rates

To understand the effect of flow variation, the clean model is run for several values of and . Typical trends include:

- Increasing either flow rate increases the capacity rate on that side, which can:
  - Increase overall heat duty ,
  - Decrease the temperature rise (or drop) of that fluid,
  - Modify slightly due to higher Reynolds number and improved convection.
- At very high flow rates, gains in may diminish due to reduced driving temperature difference, even though improves.

Such trends can be presented in plots of and versus flow rate, highlighting the trade-off between thermal performance and pumping requirements.

### 6.3 Steady Fouled Results

Steady fouled simulations show how performance changes for a given fouling level corresponding to an equivalent operating time .

#### 6.3.1 Impact on Overall Coefficient and Heat Duty

For a selected fouling scenario and time , the model yields:

- Fouling resistances .
- Deposit thicknesses .
- Fouled equivalent diameters and velocities on both sides.
- Fouled overall coefficient .
- Fouled heat duty and outlet temperatures.

It is typically observed that:

- due to the added fouling resistances; the relative drop in depends on how large fouling is compared to the clean film and wall resistances.
- For fixed inlet conditions, the heat duty decreases as fouling increases.
- The cold-side outlet temperature decreases (if the exchanger is used for heating) or increases (if used for cooling), reflecting reduced thermal performance.

A convenient way to present the effect is to normalise results:

and show these ratios as a function of operating time or fouling resistance.

#### 6.3.2 Impact on Pressure Drop

As deposits reduce the effective flow area, velocities and friction factors change, leading to modified pressure drops:

- **Tube side**: inner diameter reduction increases velocity and thus frictional losses.
- **Shell side**: narrowing of cross-flow passages increases velocity through the tube bundle and shell-side .

Results can be shown in a table or plots of and versus fouling resistance or operating time.

Typical observations:

- For moderate fouling, the **increase in** may be modest, but for heavier fouling, pressure drop can rise significantly.
- Design limits on permissible may be reached earlier than thermal performance becomes unacceptable, meaning hydraulics can drive the need for cleaning.

### 6.4 Dynamic Clean Responses

Dynamic clean simulations explore how the exchanger responds to changes in inlet conditions when surfaces are free of fouling.

#### 6.4.1 Step Change in Inlet Temperature

A common test is a **step change** in hot-side inlet temperature or cold-side inlet temperature , while keeping flow rates constant.

Typical observations from the transient response:

- The outlet temperatures and evolve smoothly from their initial steady-state values to new steady-state values.
- The response is typically **first-order-like**, with a time constant determined by the thermal capacitances and the magnitude of .
- No oscillation occurs because the underlying plant dynamics are first-order, and we are not yet applying feedback control that could introduce overshoot.

From these simulations, effective time constants can be identified (for example, by fitting simple first-order models), which is useful for controller tuning.

#### 6.4.2 Disturbance in Flow Rates

Another test applies transient changes in or . The model shows:

- Increasing at constant typically increases and shifts outlet temperatures until a new steady state is reached.
- The dynamic response again resembles a first-order system, although the direction and magnitude of change differ depending on which stream is perturbed.

Such tests demonstrate the **relative sensitivity** of outlet temperature to changes in each inlet variable, which is relevant when choosing manipulated variables for control.

### 6.5 Dynamic Fouled Behaviour

When fouling is enabled, dynamic simulations capture both **fast thermal transients** and **slow performance degradation**.

#### 6.5.1 Fouling Progression and Performance Decay

In these simulations, fouling resistance grows according to the chosen model (asymptotic or linear), and at each time step the effective overall coefficient is updated.

Typical time histories show:

- **Monotonic decrease** in as fouling builds up.
- Corresponding **reduction in heat duty** at fixed inlet conditions.
- Gradual drift of outlet temperature away from its initial design value (for example, cold outlet decreasing when the purpose is heating).

Plots of , , and versus time clearly display long-term degradation. For asymptotic models, and tend to a limiting value; for linear models, they continue degrading over the simulated horizon.

#### 6.5.2 Combined Transients and Fouling

By superimposing disturbances in inlet conditions on top of fouling progression, the simulations show:

- Short-term oscillations due to disturbances around a creeping long-term downward trend due to fouling.
- The **superposition** of fast dynamics (order of minutes) and slow dynamics (order of days/months equivalent).

This combined behaviour is representative of real plants and highlights the importance of both feedback control and long-term monitoring.

### 6.6 Closed-Loop Control Performance

Using the PID controller described in Chapter 5, the heat exchanger is operated in closed loop with the cold-side outlet temperature as the controlled variable.

#### 6.6.1 Regulatory Control - Disturbance Rejection

In the first set of tests, fouling is disabled (clean case), and a **step disturbance** in inlet conditions is applied (e.g. a step change in ).

Key results:

- The controller reacts immediately to the error and adjusts the manipulated variable (e.g. ).
- The cold outlet temperature deviates briefly but then returns to its setpoint.
- By adjusting PID gains, different responses can be obtained:
  - **Higher proportional gain** reduces steady-state error but can introduce overshoot.
  - **Integral action** removes steady-state offset but may slow down response if too aggressive.
  - **Derivative action** can help reduce overshoot and improve damping, but is sensitive to noise.

Time plots of , setpoint, and manipulated flow show how effectively the controller rejects disturbances.

#### 6.6.2 Tracking Performance - Setpoint Changes

The exchanger is then subjected to **setpoint changes** of . Typical findings:

- The closed-loop response tracks the new setpoint with acceptable rise time and overshoot, illustrating that the controller tuning is adequate for tracking tasks.
- The constraint handling (output saturation and rate limiting) is clearly visible in the manipulated-variable trajectories, ensuring physically realistic behaviour (e.g. flows cannot change infinitely fast).

These results demonstrate that the PID law, together with saturation and rate limiting, can maintain desired outlet temperatures under varying loads.

#### 6.6.3 Control Under Fouling

When fouling is re-enabled, the plant's static gain and time constant change slowly. Without adaptation, the same controller settings may:

- Produce **slower responses**,
- Increase steady-state offset if integral action is not strong enough, or
- Lead to more oscillatory behaviour in extreme cases.

Simulations with progressive fouling in closed loop show:

- The controller continues to regulate , but with **increasing control effort** as time passes (for example, higher flow rates are required to maintain the same outlet temperature).
- Eventually, physical constraints (maximum allowable flow or pressure drop) may be reached, beyond which perfect temperature regulation is no longer possible.

These results highlight that feedback control can **partially compensate** for fouling but cannot overcome fundamental thermal and hydraulic limitations.

### 6.7 Estimation Results Using RLS

The RLS estimator is applied to estimate a scalar parameter such as or an equivalent fouling parameter from online data.

#### 6.7.1 Static Case - Parameter Identification

In a first test, the plant is kept at a series of steady or slowly varying conditions (with different inlet temperatures and flows), and RLS is used to estimate a fixed parameter (e.g. ).

Typical observations:

- The estimate converges from its initial guess to a value close to the true parameter.
- The covariance decreases over time, indicating increasing confidence.
- The speed of convergence depends on the richness of excitation in (e.g. variation in the driving temperature difference).

This validates the implementation of the RLS algorithm and its ability to identify plant parameters.

#### 6.7.2 Time-Varying Case - Tracking Fouling

When fouling is enabled, the true parameter changes slowly in time. With a forgetting factor , the RLS estimator can track this variation.

Simulation results show:

- The estimated parameter (for example, ) **follows** the true value computed from the model as fouling increases.
- A small lag is present, determined by the choice of and the amount of noise or disturbances.
- Too small a forgetting factor causes the estimate to fluctuate excessively; too large a value makes the estimator too slow to respond.

Plots of the **true** and **estimated** parameter versus time clearly demonstrate the trade-off and the ability of RLS to track gradual degradation.

These results are important because they show that **online monitoring** of exchanger health is feasible: from routine operating data, the system can infer how far the exchanger has deviated from its clean condition.

### 6.8 Numerical Robustness and Validation

The numerical implementation includes basic checks to ensure robust simulation:

- Outputs are checked for finite values (no NaNs or infinities).
- Temperature updates are limited within physically reasonable bounds (e.g. 0-200 °C).
- Time steps are chosen to maintain stability of the explicit Euler scheme.

Model validation can be carried out at two levels:

- **Analytical/benchmark checks**:
  - Comparing steady clean results with hand calculations using standard design formulas.
  - Verifying limiting cases (e.g. zero flow, equal inlet temperatures) against expected physical behaviour.
- **Data-based validation** (if experimental or plant data is available):
  - Matching predicted outlet temperatures and heat duty against measured values for various operating conditions.
  - Comparing estimated fouling trends with inspection or cleaning records.

Although the present project primarily focuses on modelling and simulation, the structure is ready to incorporate real data for further validation.

### 6.9 Summary of Key Insights

The main insights from the simulation, control and estimation studies are:

- **Steady clean** simulations provide a baseline thermal and hydraulic performance that matches expectations from standard design correlations.
- **Steady fouled** simulations show significant degradation in , heat duty and outlet temperature, and an increase in pressure drop as fouling increases.
- **Dynamic clean** simulations confirm that the exchanger behaves like a first-order system in response to inlet disturbances, with time constants governed by fluid holdups and .
- **Dynamic fouled** simulations reveal the combined effect of short-term disturbances and long-term performance decay, mimicking real plant behaviour.
- The **PID controller** can maintain the cold-side outlet temperature close to its setpoint under a range of disturbances, but its effectiveness is ultimately constrained by physical limits as fouling progresses.
- The **RLS estimator** successfully identifies and tracks time-varying parameters linked to fouling, enabling online performance monitoring and potential integration into adaptive or predictive maintenance strategies.

Taken together, these results demonstrate that the developed HeatXTwin Pro framework is not only capable of predicting heat-exchanger performance in detail, but also of supporting advanced control and estimation schemes for real-time operation and long-term asset management.

If you'd like, the **next chapter** can be a concise **Conclusion and Future Work** (final chapter), or we can also add a short separate chapter about the **HeatXTwin Pro GUI / software architecture** if your college expects a software implementation chapter.

add a short separate chapter about the HeatXTwin Pro GUI / software architecture just focus on core knowledge more because it is a mechanical project

## CHAPTER 7

## SOFTWARE ARCHITECTURE AND HEATXTWIN PRO GUI

Although this is primarily a mechanical engineering project, the **HeatXTwin Pro** software is an essential part of the work. It provides a practical interface to the mathematical model and simulations described in earlier chapters, allowing a user to:

- Enter geometry, fluid properties and operating conditions,
- Select simulation modes (steady/dynamic, clean/fouled),
- Run simulations and visualise key mechanical/thermal quantities,
- Experiment with control and estimation parameters.

This chapter briefly describes the **software architecture** and the **graphical user interface (GUI)**, while keeping the focus on how they support mechanical analysis rather than on programming details.

### 7.1 Purpose of the Software Layer

From a mechanical-engineering point of view, the software has three main functions:

- **Encapsulate the mathematical model**  
    The C++ backend implements the full shell-and-tube heat exchanger model (heat transfer, fouling, hydraulics, dynamics, PID control, RLS estimation) in a consistent and reusable way.
- **Provide an engineering "laboratory"**  
    The GUI lets a user change geometry, operating conditions, fouling parameters, and controller settings and immediately see the effect on:
  - Outlet temperatures,
  - Heat duty and effectiveness,
  - Overall heat-transfer coefficient,
  - Tube- and shell-side pressure drops,
  - Fouling growth over time.
- **Bridge theory and intuition**  
    Instead of only solving equations by hand or in a spreadsheet, the user can interact with a **digital twin** that behaves like a real exchanger. This is especially valuable for understanding trade-offs between heat-transfer performance, pressure drop, and fouling.

### 7.2 High-Level Software Architecture

At a high level, HeatXTwin Pro follows a **layered architecture**:

- **User Interface Layer (GUI - Qt MainWindow)**
  - Created using the Qt framework (entry point in main.cpp).
  - Provides forms, buttons, and plots for user interaction.
  - Reads user inputs (geometry, fluid data, operating point, PID gains, fouling model choice).
  - Displays simulation results in numeric and graphical form.
- **Application / Coordination Layer**
  - Connects GUI actions (e.g. "Run Simulation") to the backend.
  - Builds the necessary data structures (Geometry, Fluid, OperatingPoint, FoulingParams, Limits, controller/estimator settings).
  - Chooses the appropriate simulation mode (steady clean, steady fouled, dynamic clean, dynamic fouled).
  - Calls the Model or Simulator classes and receives results.
- **Simulation and Modelling Layer (Core C++ backend)**
  - **Types** - Defines all fundamental structures:  
        Fluid, Geometry, OperatingPoint, FoulingParams, Limits, State.
  - **Thermo** - Implements heat-transfer correlations and ε-NTU steady model.
  - **Fouling** - Implements asymptotic and linear fouling models (resistance and thickness).
  - **Hydraulics** - Computes tube- and shell-side pressure drops from geometry and flow.
  - **Model** - Ties Thermo, Fouling, and Hydraulics together for a single operating point and time.
  - **Simulator** - Integrates the dynamic model in time for clean or fouled cases.
  - **ControllerPID** - Discrete PID controller with saturation and rate limit.
  - **EstimatorRLS** - Scalar RLS estimator for online parameter estimation.
  - **Validation** - Basic numerical checks (finite values, bounds).

This separation ensures that mechanical formulas and correlations are **isolated** inside Thermo, Fouling, and Hydraulics, while the GUI remains a thin layer focused on visualization and user input.

### 7.3 Typical Data Flow for a Simulation

From the user's perspective, running a simulation involves a few simple steps. Internally, a clear data flow occurs.

#### 7.3.1 Steady-State Simulation (Clean or Fouled)

- **User Input (GUI)**  
    The user enters:
  - Tube dimensions, shell size, number of tubes, baffle spacing (geometry),
  - Fluid properties or fluid type and operating temperatures,
  - Mass flow rates on hot and cold sides,
  - Fouling model and parameters (if steady fouled),
  - Design limits for pressure drop, etc.
- **Construction of Data Structures**  
    The application layer builds:
  - Geometry from tube and shell dimensions,
  - Fluid objects for hot and cold streams,
  - OperatingPoint from flow rates and inlet temperatures,
  - FoulingParams describing asymptotic or linear fouling,
  - Limits for Δp and flow rates.
- **Call to Model**
  - For a **clean** case: fouling model is disabled (Rf = 0).
  - For a **fouled** case: fouling resistance is evaluated at the chosen operating time .
  - Model::evaluate(op, t_seconds) uses Thermo, Fouling, and Hydraulics to compute a State containing  
        .
- **Results to GUI**  
    The GUI receives the State and displays:
  - Numerical values in tables (temperatures, heat duty, U, Δp),
  - Optionally, plots comparing clean vs fouled cases or showing sensitivity to a parameter (e.g., flow rate).

#### 7.3.2 Dynamic Simulation (Clean or Fouled)

- **User Input (GUI)**  
    Additional settings include:
  - Simulation time step and horizon ,
  - Initial operating point,
  - Whether to enable fouling growth,
  - Whether to enable PID control and RLS estimation (if these features are used).
- **Simulator Setup**  
    The application layer configures a Simulator object with:
  - Geometry, fluids, fouling parameters, limits,
  - Initial operating point and holdup masses ,
  - Mode flags: steady-state mode vs full dynamic, fouling enabled/disabled.
- **Time Loop**  
    For each time step:
  - Simulator::step(t) updates fouling (if enabled), recomputes ,
  - Integrates outlet temperatures using the dynamic energy balances,
  - Recomputes pressure drops,
  - Optionally applies PID control logic to adjust mass flow rate,
  - Optionally feeds measurements to the RLS estimator to update parameter estimates,
  - Stores results for plotting.
- **Visualization**  
    The GUI plots time histories such as:
  - , ,
  - and ,
  - Fouling resistance ,
  - Pressure drops and manipulated variable (flow rate) if control is active.

This pipeline allows the mechanical engineer to see **how theory translates into time-domain behaviour** and how fouling and control interact.

### 7.4 Role of the GUI for Mechanical Analysis

Even though the code is written in C++, the real value for a mechanical project is how the GUI helps in **understanding and using the model**:

- **Visual comparison of scenarios**  
    The user can quickly compare:
  - Clean vs fouled performance,
  - Effect of different fouling models (asymptotic vs linear),
  - Influence of changing geometry (e.g. tube diameter, length, number of tubes) on , , and Δp.
- **Investigation of trade-offs**  
    The GUI makes it easy to observe classic design trade-offs:
  - Increasing flow rate improves heat transfer but increases pressure drop.
  - Tightening baffle spacing improves shell-side heat transfer but raises shell-side Δp.
  - Allowing more fouling margin in design (higher clean U) can delay unacceptable performance but may overdesign the exchanger.
- **Control and estimation insight**  
    With the PID + RLS features active, the GUI helps to:
  - See how a simple controller can hold outlet temperature despite disturbances,
  - Observe how much extra pumping effort is required as fouling worsens,
  - Watch the RLS estimator track a changing parameter, illustrating the concept of online performance monitoring.
- **Teaching and demonstration tool**  
    For academic use, HeatXTwin Pro acts as an interactive teaching tool:
  - Students can experiment with parameters themselves and immediately see the consequences.
  - Concepts such as NTU-effectiveness, fouling resistance, pressure drop, thermal capacitance, and dynamic response are reinforced by direct simulation instead of only by equations.

In summary, the GUI is not a cosmetic add-on; it is a **practical interface** to the mechanical model that enhances understanding and allows systematic exploration of design and operation.

### 7.5 Summary

This chapter has outlined the **software architecture** and **graphical user interface** of HeatXTwin Pro from a mechanical-engineering perspective:

- The system is organised in layers: a Qt-based GUI, an application/coordination layer, and a core simulation backend.
- The backend encapsulates all mechanical and thermal modelling in well-defined modules (Thermo, Fouling, Hydraulics, Model, Simulator, ControllerPID, EstimatorRLS).
- The GUI collects input data, triggers steady and dynamic simulations, and visualises the resulting temperatures, heat duties, coefficients, pressure drops, and control/estimation signals.
- This structure turns the theoretical heat-exchanger model into a usable **digital twin**, helping engineers and students to explore design options, understand dynamic behaviour, and appreciate the impact of fouling and control on real equipment.

By keeping the core physics and correlations in the backend and using the GUI mainly for interaction and visualisation, the project maintains a strong mechanical focus while leveraging modern software tools to make analysis and experimentation more intuitive.

final chapter

## CHAPTER 8

## CONCLUSIONS AND FUTURE WORK

### 8.1 Summary of Work

This project presented the development of a **digital twin-oriented simulation framework** for a **shell-and-tube heat exchanger**, with explicit consideration of:

- Steady and dynamic thermal behaviour,
- Fouling growth and its impact on performance,
- Hydraulic (pressure-drop) characteristics,
- Closed-loop temperature control using PID, and
- Online parameter estimation using Recursive Least Squares (RLS).

On the modelling side, a **unified mathematical framework** was formulated that can operate in four modes:

- Steady clean,
- Steady fouled,
- Dynamic clean, and
- Dynamic fouled.

The model incorporates standard correlations for **tube-side and shell-side heat transfer**, a **series thermal-resistance network** for overall heat-transfer coefficients, **asymptotic and linear fouling models**, and **Darcy-Weisbach-based pressure-drop relations** for both tube and shell circuits.

On the software side, these equations were implemented in C++ in a modular fashion. The **HeatXTwin Pro** application provides a Qt-based GUI that allows users to:

- Specify geometry, fluid properties, and operating conditions,
- Choose simulation modes and fouling models,
- Run steady or dynamic simulations, and
- Visualize outlet temperatures, heat duty, overall coefficient, pressure drop, fouling level, and control signals.

A **PID controller** was added to regulate the cold-side outlet temperature, and a **scalar RLS estimator** was used to track a key parameter (such as ) online, representing the impact of fouling. Simulation studies were carried out to evaluate clean versus fouled behaviour, transient responses, disturbance rejection, and the performance of control and estimation strategies.

### 8.2 Key Technical Findings

From the modelling and simulation studies, several important mechanical insights were obtained:

- **Shell-and-tube exchangers behave approximately as first-order systems**  
    Under single-phase, lumped-parameter assumptions, the exchanger's thermal response to inlet disturbances is well approximated by first-order dynamics. The time constants are governed by:
  - Fluid holdups on hot and cold sides, and
  - The effective conductance .

- This justifies the use of relatively simple PID controllers for outlet temperature control.

- **Fouling significantly reduces effective and heat duty**  
    Inclusion of realistic fouling resistances (either asymptotic or linear) always leads to:
  - A reduction in overall heat-transfer coefficient ,
  - A corresponding reduction in heat duty for fixed inlet conditions, and
  - Drift in outlet temperatures away from design values.

- The relative drop in depends strongly on how large the fouling resistances are compared to clean film and wall resistances.

- **Hydraulic effects of fouling can be just as critical as thermal effects**  
    As deposits build up, effective flow areas shrink, velocities increase, and frictional losses grow. In many conditions, **allowable pressure-drop limits may be reached before the thermal performance becomes completely unacceptable**, implying that cleaning decisions cannot be based on temperature alone.
- **Simple PID control can compensate for moderate disturbances and mild fouling**  
    A properly tuned PID controller was able to:
  - Reject step disturbances in inlet temperature and flow reasonably well, and
  - Maintain the cold-side outlet temperature near its setpoint for a wide range of conditions.

- As fouling progresses, the controller naturally increases the manipulated flow rate to compensate for reduced . This illustrates how feedback control can **extend the useful operating period** of an exchanger.

- **Control action is ultimately limited by physical and hydraulic constraints**  
    Under heavy fouling, the controller approaches actuator and hydraulic limits (maximum flow rate and maximum allowable Δp). Beyond this point, setpoint tracking becomes impossible, even with ideal tuning. This highlights the importance of **integrating control analysis with hydraulic and mechanical design constraints**.
- **RLS estimation can track gradual degradation of**  
    The scalar RLS estimator, driven by measured or computed heat duty and temperature differences, can follow slow changes in an effective conductance parameter. With an appropriate forgetting factor:
  - The estimated converges under clean, time-invariant conditions, and
  - Tracks the decline in when fouling is enabled.

- This demonstrates the feasibility of **online performance monitoring**, which could be used for predictive maintenance or adaptive control.

### 8.3 Contributions of the Project

The main contributions of this work are:

- **Unified modelling framework**  
    A single, coherent set of equations was developed that can represent steady/dynamic and clean/fouled operation without changing the core structure. Switching between modes is achieved by enabling/disabling fouling and choosing steady versus dynamic solution methods.
- **Explicit coupling of thermal, fouling and hydraulic phenomena**  
    The model accounts not only for the additional thermal resistance from fouling, but also for **geometry changes** (effective diameters, flow areas) and their influence on Reynolds numbers, Nusselt numbers, and pressure drops. This gives a more realistic picture of long-term operation.
- **Integration of control and estimation into a mechanical model**  
    By embedding PID control and RLS estimation into the exchanger model, the project bridges classical heat-transfer design with modern **process control and condition monitoring**, which is increasingly relevant in industrial practice.
- **Interactive digital twin environment (HeatXTwin Pro)**  
    The software provides a user-friendly interface for mechanical engineers and students to explore:
  - The effect of geometry, fouling, and operating conditions,
  - Trade-offs between heat transfer and pressure drop,
  - Closed-loop behaviour and online estimation.

- This transforms the theoretical model into a practical engineering tool.

### 8.4 Limitations

While the developed framework is comprehensive for a student project, certain limitations should be acknowledged:

- **Lumped-parameter approximation**  
    The exchanger is modelled as two lumped control volumes (one for each fluid). Axial and radial temperature gradients are not resolved. For very long exchangers or highly non-uniform conditions, a distributed (multi-cell) model would be more accurate.
- **Single-phase, single-component fluids**  
    Phase change (boiling or condensation), multi-component effects, and strong property variations with temperature are not considered. Industrial exchangers often involve such complexities.
- **Empirical correlations and validity ranges**  
    Tube-side and shell-side heat transfer and pressure-drop correlations are empirical and valid only for certain geometries and Reynolds/Prandtl ranges. Outside these ranges, predictions may be less accurate.
- **Simplified fouling models**  
    Fouling is represented using simple asymptotic or linear resistances with uniform coverage. Real fouling may depend on local wall temperature, concentration, shear stress, and may be highly non-uniform. Chemical reaction fouling, biofouling, or particulate deposition mechanisms are not separately modelled.
- **No explicit model of actuator and sensor dynamics**  
    The control layer assumes that mass flow can change according to the controller output with only rate and amplitude limits. Real valves and pumps have additional dynamics, nonlinearities, and potential dead zones.
- **Lack of experimental validation in this work**  
    The current evaluation relies on theoretical expectations and internal consistency. Validation against plant or lab data would be required before using the model for design decisions in industry.

### 8.5 Scope for Future Work

There are several promising directions in which this project can be extended:

- **Experimental validation and calibration**
  - Build or access a lab-scale shell-and-tube heat exchanger and perform controlled experiments.
  - Fit model parameters (fouling rates, deposit conductivity, friction factors) to measured data.
  - Quantify model accuracy and refine correlations where necessary.
- **Distributed (multi-cell) modelling**
  - Extend the dynamic model to multiple segments along the exchanger length (finite volume approach).
  - Capture axial temperature profiles, maldistribution, and entrance effects.
  - Study the impact of non-uniform fouling along the length.
- **More advanced fouling and cleaning models**
  - Implement threshold or mechanistic fouling models incorporating wall temperature, fluid velocity, and concentration effects.
  - Include models for **cleaning cycles** (mechanical or chemical), allowing simulation of long-term operation over many fouling/cleaning intervals.
  - Couple fouling predictions to economic metrics (energy loss vs cleaning cost).
- **Enhanced control strategies**
  - Implement **feed-forward control** using measured disturbances (e.g. hot inlet temperature) to improve disturbance rejection.
  - Explore **Model Predictive Control (MPC)** for managing constraints (maximum Δp, maximum flow) explicitly.
  - Use the RLS estimates to realise **adaptive PID** or gain scheduling based on current .
- **Extended estimator design**
  - Generalise RLS to a vector formulation to estimate multiple parameters simultaneously (e.g. separate tube- and shell-side fouling coefficients or time constants).
  - Incorporate measurement noise models and robust estimation techniques (e.g. Kalman filtering).
- **Integration into plant-wide digital twin frameworks**
  - Connect HeatXTwin Pro to other unit models (pumps, reactors, distillation columns) to build a **plant-wide simulation**.
  - Use such a digital twin for operator training, what-if analysis, and optimization studies.
- **Industrial-grade software enhancements**
  - Add data import/export to interface with plant historians or lab data acquisition systems.
  - Improve GUI features such as report generation, parameter sweep tools, and automated sensitivity analysis.
  - Package the software as a teaching tool for heat-transfer and process-control courses.

### 8.6 Concluding Remarks

This project demonstrates that a **mechanically grounded, yet control-aware digital twin** of a shell-and-tube heat exchanger can be built using classical heat-transfer theory, practical fouling and hydraulic models, and relatively simple control and estimation algorithms.

By combining analytical modelling with a robust software implementation (HeatXTwin Pro), the work moves beyond static design calculations and provides a platform to:

- Understand dynamic behaviour,
- Explore the interaction between fouling, performance, and pressure drop, and
- Investigate how feedback control and online estimation can support efficient, reliable operation over the equipment's life.

In an industrial context where **energy efficiency**, **reliability**, and **predictive maintenance** are increasingly important, such integrated modelling and simulation tools are likely to become standard in the design and operation of thermal systems. This project represents a step in that direction and provides a solid foundation for further research and practical applications.

