
## EMA/RMS realtime-tuning parameters

The gate in [SmartGate.cpp](VocalCleanupPlugin/Source/SmartGate.cpp#L42-L48) turns each input sample $x[n]$ into an open/close decision in three stages. Every stage is an exponential moving average (EMA) of the same shape — only the exponents and time constants differ — and every knob below maps to one symbol in the equations. All setters take effect live; the time-based ones recompute their coefficient from the sample rate $f_s$.

### Stage 1 — Level detector (generalized power-mean)

The detector is a **generalized power-mean**: raise $|x|$ to a power $p$, smooth it with an EMA, then take the $r$-th root.

$$L[n] = \Big(\underbrace{\text{EMA}\big(|x[n]|^{\,p}\big)}_{\text{Mean}}\Big)^{\,r}$$

Expanded, the running EMA state $s[n]$ and the level $L[n]$ are:

$$s[n] = s[n-1] + \alpha\big(|x[n]|^{\,p} - s[n-1]\big), \qquad L[n] = s[n]^{\,r}$$

with the smoothing factor derived from the averaging time $\tau$:

$$\alpha = 1 - e^{-1/(\tau f_s)}$$

The level is then converted to dB and clamped at a floor $F$ to give the detector reading $\ell[n]$:

$$\ell[n] = \max\!\big(20\log_{10} L[n],\; F\big)$$

| Symbol | Variable | Setter | Default | Meaning |
|---|---|---|---|---|
| $p$ | `emaPower` | `setEmaPower()` | `2.0` | Exponent on $\lvert x\rvert$ before averaging. `2` = mean-square. |
| $r$ | `rmsRootPower` | `setRmsRootPower()` | `0.5` | Root applied to the smoothed value. `0.5` = √ → true RMS. |
| $\tau$ | `levelTimeSec` | `setLevelTime()` | `0.01` (~10 ms) | Averaging window; sets $\alpha$ via the formula above. |
| $\alpha$ | `levelAlpha` | (computed) | from $\tau,f_s$ | EMA smoothing factor (0–1); not set directly. |
| $s[n]$ | `smoothedSquares` | (state) | — | Running EMA of $\lvert x\rvert^{\,p}$. |
| $L[n]$ | (local `level`) | (derived) | — | Current signal level $s[n]^{\,r}$ in linear gain (not the noise floor). Its dB form $\ell[n]$ is stored as `lastBlockDb` / `getSignalDb()`. |
| $F$ | `detectorFloorDb` | `setDetectorFloor()` | `-100.0` | Lower dB clamp for the level/floor conversion. |

The exponents being variables costs nothing at the defaults — `(p = 2, r = 0.5)` is exactly RMS — but turns the fixed formula into a knob-able detector:

- `(p = 2, r = 0.5)` → **true RMS** (energy/loudness; the default).
- `(p = 1, r = 1)` → EMA of $|x|$, an *average-rectified* detector: faster and peakier, tracks amplitude rather than energy.
- Other values → experimental detector responses.

### Stage 2 — Adaptive noise floor (asymmetric EMA)

The floor $N[n]$ tracks $\ell[n]$ with **two different time constants** so it drops quickly toward a falling signal but rises slowly, letting brief loud sounds stand out above it:

$$N[n] = \begin{cases} \ell[n] + a_{\text{drop}}\,\big(N[n-1] - \ell[n]\big), & \ell[n] < N[n-1] \quad(\text{signal below floor} \to \text{fall}) \\[4pt] \ell[n] + a_{\text{climb}}\,\big(N[n-1] - \ell[n]\big), & \ell[n] \ge N[n-1] \quad(\text{signal above floor} \to \text{rise}) \end{cases}$$

with per-sample retention coefficients

$$a_{\text{drop}} = e^{-1/(\tau_{\text{drop}} f_s)}, \qquad a_{\text{climb}} = e^{-1/(\tau_{\text{climb}} f_s)}$$

| Symbol | Variable | Setter | Default | Meaning |
|---|---|---|---|---|
| $\tau_{\text{drop}}$ | `dropTimeSec` | `setDropTime()` | `0.05` | Floor **fall** time constant → $a_{\text{drop}}$. Small = tracks down fast. |
| $\tau_{\text{climb}}$ | `climbTimeSec` | `setClimbTime()` | `3.0` | Floor **rise** time constant → $a_{\text{climb}}$. Large = rises slowly. |
| $a_{\text{drop}}$ | `dropCoef` | (computed) | from $\tau_{\text{drop}},f_s$ | Retention on the fall branch. |
| $a_{\text{climb}}$ | `climbCoef` | (computed) | from $\tau_{\text{climb}},f_s$ | Retention on the rise branch. |
| $N[n]$ | `currentNoiseFloorDb` | (state) | `-100.0` | Estimated noise floor, dB. |

### Stage 3 — Active threshold and gate decision

The gate opens when the level rises a fixed offset $\Theta$ above the tracked floor. A **hangover** counter $h$ holds the gate open for $H$ samples after the level drops back, preventing chatter:

$$T[n] = N[n] + \Theta$$

$$\text{open}[n] = \begin{cases} \text{true}, \;\; h \leftarrow H & \ell[n] > T[n] \quad(\text{re-trigger}) \\[4pt] \text{true}, \;\; h \leftarrow h-1 & \ell[n] \le T[n],\; h > 0 \quad(\text{hangover}) \\[4pt] \text{false} & \text{otherwise} \end{cases}$$

where the hold length is $H = \operatorname{round}(\text{hangoverTimeMs}\cdot f_s / 1000)$. When `open` is false the sample is zeroed on every channel.

| Symbol | Variable | Setter | Default | Meaning |
|---|---|---|---|---|
| $\Theta$ | `thresholdOffsetDb` | `setThresholdOffset()` | `12.0` | dB above the floor required to open the gate. |
| $T[n]$ | `activeThresholdDb` | (state) | — | Current open threshold, $N[n] + \Theta$. |
| $H$ | `hangoverDurationSamples` | `setHangoverTime()` (ms) | from `100` ms | Samples the gate stays open after level falls below $T$. |
| $h$ | `hangoverCounter` | (state) | — | Remaining hangover samples. |
